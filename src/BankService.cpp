#include "BankService.hpp"
#include "DatabaseManager.hpp"
#include "AccountFactory.hpp"
#include "AuthManager.hpp"
#include "Exceptions.hpp"
#include "Utils.hpp"

// ----------------------------------------------------------------------------
// Customer management
// ----------------------------------------------------------------------------

int BankService::registerCustomer(const std::string& name, const std::string& email,
                                   const std::string& phone, const std::string& address) {
    if (name.empty() || email.empty()) {
        throw BankException("Customer name and email are required.");
    }
    Customer newCustomer(-1, name, email, phone, address);
    return DatabaseManager::getInstance().createCustomer(newCustomer);
}

Customer BankService::getCustomer(int customerId) {
    return DatabaseManager::getInstance().readCustomer(customerId);
}

std::vector<Customer> BankService::listCustomers() {
    return DatabaseManager::getInstance().readAllCustomers();
}

bool BankService::updateCustomer(const Customer& customer) {
    return DatabaseManager::getInstance().updateCustomer(customer);
}

bool BankService::removeCustomer(int customerId) {
    return DatabaseManager::getInstance().deleteCustomer(customerId);
}

// ----------------------------------------------------------------------------
// Account management
// ----------------------------------------------------------------------------

int BankService::openAccount(int customerId, const std::string& accountType,
                              double initialDeposit, const std::string& pin) {
    if (initialDeposit < 0) {
        throw InvalidAmountException("Initial deposit cannot be negative.");
    }
    // Validate the customer exists before opening an account for them.
    DatabaseManager::getInstance().readCustomer(customerId);

    std::string pinHash = Utils::hashPin(pin);
    // AccountFactory is used purely to validate the type string and to
    // build a transient in-memory object matching the type's business
    // rules (e.g. rejecting an initial deposit below the minimum balance)
    // before it is persisted.
    std::unique_ptr<Account> account = AccountFactory::createAccount(
        accountType, /*accountId placeholder*/ -1, customerId, initialDeposit, pinHash);

    return DatabaseManager::getInstance().createAccount(*account);
}

std::unique_ptr<Account> BankService::getAccount(int accountId) {
    return DatabaseManager::getInstance().readAccount(accountId);
}

std::vector<std::unique_ptr<Account>> BankService::listAccountsForCustomer(int customerId) {
    return DatabaseManager::getInstance().readAccountsForCustomer(customerId);
}

bool BankService::closeAccount(int accountId) {
    return DatabaseManager::getInstance().deleteAccount(accountId);
}

// ----------------------------------------------------------------------------
// Core banking operations
// ----------------------------------------------------------------------------

void BankService::deposit(int accountId, double amount, const std::string& pin) {
    auto& db = DatabaseManager::getInstance();
    std::unique_ptr<Account> account = db.readAccount(accountId);

    AuthManager::authenticate(*account, pin);
    account->deposit(amount); // validates amount > 0 internally

    if (!db.updateAccountBalance(accountId, account->getBalance())) {
        throw DatabaseException("Failed to persist updated balance after deposit.");
    }

    Transaction txn(-1, accountId, TransactionType::DEPOSIT, amount,
                     account->getBalance(), Utils::currentTimestamp(), "Cash deposit");
    db.recordTransaction(txn);
}

void BankService::withdraw(int accountId, double amount, const std::string& pin) {
    auto& db = DatabaseManager::getInstance();
    std::unique_ptr<Account> account = db.readAccount(accountId);

    AuthManager::authenticate(*account, pin);
    account->withdraw(amount); // enforces per-account-type minimum balance / overdraft rule

    if (!db.updateAccountBalance(accountId, account->getBalance())) {
        throw DatabaseException("Failed to persist updated balance after withdrawal.");
    }

    Transaction txn(-1, accountId, TransactionType::WITHDRAWAL, amount,
                     account->getBalance(), Utils::currentTimestamp(), "Cash withdrawal");
    db.recordTransaction(txn);
}

void BankService::transfer(int fromAccountId, int toAccountId, double amount, const std::string& pin) {
    if (fromAccountId == toAccountId) {
        throw InvalidAccountException("Cannot transfer to the same account.");
    }

    auto& db = DatabaseManager::getInstance();
    std::unique_ptr<Account> fromAccount = db.readAccount(fromAccountId);
    std::unique_ptr<Account> toAccount = db.readAccount(toAccountId);

    AuthManager::authenticate(*fromAccount, pin);

    // Apply both legs in memory first so that if either violates a
    // business rule (e.g. insufficient funds), nothing has been persisted
    // yet and the database stays consistent.
    fromAccount->withdraw(amount);
    toAccount->deposit(amount);

    if (!db.updateAccountBalance(fromAccountId, fromAccount->getBalance())) {
        throw DatabaseException("Failed to debit source account during transfer.");
    }
    if (!db.updateAccountBalance(toAccountId, toAccount->getBalance())) {
        // Best-effort rollback of the debit so the source account isn't
        // left short if the credit leg fails.
        db.updateAccountBalance(fromAccountId, fromAccount->getBalance() + amount);
        throw DatabaseException("Failed to credit destination account during transfer; transfer rolled back.");
    }

    std::string timestamp = Utils::currentTimestamp();
    db.recordTransaction(Transaction(-1, fromAccountId, TransactionType::TRANSFER_OUT, amount,
                                      fromAccount->getBalance(), timestamp,
                                      "Transfer to account #" + std::to_string(toAccountId)));
    db.recordTransaction(Transaction(-1, toAccountId, TransactionType::TRANSFER_IN, amount,
                                      toAccount->getBalance(), timestamp,
                                      "Transfer from account #" + std::to_string(fromAccountId)));
}

std::vector<Transaction> BankService::getTransactionHistory(int accountId) {
    return DatabaseManager::getInstance().readTransactionsForAccount(accountId);
}

double BankService::checkBalance(int accountId, const std::string& pin) {
    std::unique_ptr<Account> account = DatabaseManager::getInstance().readAccount(accountId);
    AuthManager::authenticate(*account, pin);
    return account->getBalance();
}
