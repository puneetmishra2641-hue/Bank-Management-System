#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Customer.hpp"
#include "Account.hpp"
#include "Transaction.hpp"

// ============================================================================
// BankService.hpp
// Business-logic facade sitting between the console UI (main.cpp) and the
// persistence layer (DatabaseManager). Coordinates multi-step operations
// (e.g. fund transfer = withdraw from one account + deposit into another +
// two ledger entries) so those steps are never scattered across the UI.
// ============================================================================

class BankService {
public:
    // --- Customer management ---
    int registerCustomer(const std::string& name, const std::string& email,
                          const std::string& phone, const std::string& address);
    Customer getCustomer(int customerId);
    std::vector<Customer> listCustomers();
    bool updateCustomer(const Customer& customer);
    bool removeCustomer(int customerId);

    // --- Account management ---
    int openAccount(int customerId, const std::string& accountType,
                     double initialDeposit, const std::string& pin);
    std::unique_ptr<Account> getAccount(int accountId);
    std::vector<std::unique_ptr<Account>> listAccountsForCustomer(int customerId);
    bool closeAccount(int accountId);

    // --- Core banking operations ---
    void deposit(int accountId, double amount, const std::string& pin);
    void withdraw(int accountId, double amount, const std::string& pin);
    void transfer(int fromAccountId, int toAccountId, double amount, const std::string& pin);

    std::vector<Transaction> getTransactionHistory(int accountId);
    double checkBalance(int accountId, const std::string& pin);
};
