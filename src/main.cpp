#include <iostream>
#include <limits>
#include <iomanip>
#include "BankService.hpp"
#include "DatabaseManager.hpp"
#include "AccountFactory.hpp"
#include "Exceptions.hpp"
#include "DatabaseConfig.hpp"

// main.cpp
// Console entry point. Owns a single BankService instance and dispatches
// menu choices to it. All exceptions raised anywhere in the call stack
// (DatabaseException, InsufficientFundsException, AuthenticationException,
// etc.) are caught here at the top level so the console loop never crashes
// on bad input or a business-rule violation -- it prints the message and
// returns to the menu instead.

namespace
{

    void clearInputBuffer()
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    int readInt(const std::string &prompt)
    {
        int value;
        std::cout << prompt << std::flush;
        while (!(std::cin >> value))
        {
            std::cout << "Invalid input. Please enter a number: " << std::flush;
            clearInputBuffer();
        }
        return value;
    }

    double readDouble(const std::string &prompt)
    {
        double value;
        std::cout << prompt << std::flush;
        while (!(std::cin >> value))
        {
            std::cout << "Invalid input. Please enter a number: " << std::flush;
            clearInputBuffer();
        }
        return value;
    }

    std::string readLine(const std::string &prompt)
    {
        std::string value;
        std::cout << prompt << std::flush;
        std::cin >> std::ws;
        std::getline(std::cin, value);
        return value;
    }

    void printMenu()
    {
        std::cout << "\n===================== BANK MANAGEMENT SYSTEM =====================\n"
                  << " 1. Register New Customer\n"
                  << " 2. Open Account\n"
                  << " 3. Deposit\n"
                  << " 4. Withdraw\n"
                  << " 5. Transfer Funds\n"
                  << " 6. Check Balance\n"
                  << " 7. Transaction History\n"
                  << " 8. View Customer Details\n"
                  << " 9. List All Customers\n"
                  << "10. Close Account\n"
                  << " 0. Exit\n"
                  << "====================================================================\n"
                  << "Enter choice: " << std::flush;
    }

    void handleRegisterCustomer(BankService &service)
    {
        std::string name = readLine("Name: ");
        std::string email = readLine("Email: ");
        std::string phone = readLine("Phone: ");
        std::string address = readLine("Address: ");
        int id = service.registerCustomer(name, email, phone, address);
        std::cout << "Customer registered successfully. Customer ID: " << id << "\n";
    }

    void handleOpenAccount(BankService &service)
    {
        int customerId = readInt("Customer ID: ");
        std::string type = readLine("Account Type (Savings/Current): ");
        double initialDeposit = readDouble("Initial Deposit: ");
        std::string pin = readLine("Set 4-digit PIN: ");
        int accId = service.openAccount(customerId, type, initialDeposit, pin);
        std::cout << "Account opened successfully. Account ID: " << accId << "\n";
    }

    void handleDeposit(BankService &service)
    {
        int accountId = readInt("Account ID: ");
        double amount = readDouble("Deposit Amount: ");
        std::string pin = readLine("PIN: ");
        service.deposit(accountId, amount, pin);
        std::cout << "Deposit successful.\n";
    }

    void handleWithdraw(BankService &service)
    {
        int accountId = readInt("Account ID: ");
        double amount = readDouble("Withdrawal Amount: ");
        std::string pin = readLine("PIN: ");
        service.withdraw(accountId, amount, pin);
        std::cout << "Withdrawal successful.\n";
    }

    void handleTransfer(BankService &service)
    {
        int fromId = readInt("From Account ID: ");
        int toId = readInt("To Account ID: ");
        double amount = readDouble("Amount: ");
        std::string pin = readLine("PIN for source account: ");
        service.transfer(fromId, toId, amount, pin);
        std::cout << "Transfer completed successfully.\n";
    }

    void handleCheckBalance(BankService &service)
    {
        int accountId = readInt("Account ID: ");
        std::string pin = readLine("PIN: ");
        double balance = service.checkBalance(accountId, pin);
        std::cout << std::fixed << std::setprecision(2)
                  << "Current Balance: " << balance << "\n";
    }

    void handleTransactionHistory(BankService &service)
    {
        int accountId = readInt("Account ID: ");
        auto history = service.getTransactionHistory(accountId);
        if (history.empty())
        {
            std::cout << "No transactions found for this account.\n";
            return;
        }
        std::cout << std::left << std::setw(6) << "ID" << std::setw(15) << "Type"
                  << std::setw(12) << "Amount" << std::setw(14) << "Balance After"
                  << std::setw(21) << "Timestamp" << "Remarks\n";
        std::cout << std::string(80, '-') << "\n";
        for (const auto &txn : history)
        {
            std::cout << std::left << std::setw(6) << txn.getTransactionId()
                      << std::setw(15) << Transaction::typeToString(txn.getType())
                      << std::setw(12) << std::fixed << std::setprecision(2) << txn.getAmount()
                      << std::setw(14) << txn.getBalanceAfter()
                      << std::setw(21) << txn.getTimestamp()
                      << txn.getRemarks() << "\n";
        }
    }

    void handleViewCustomer(BankService &service)
    {
        int customerId = readInt("Customer ID: ");
        Customer customer = service.getCustomer(customerId);
        customer.display();

        auto accounts = service.listAccountsForCustomer(customerId);
        std::cout << "\nAccounts:\n";
        for (const auto &acc : accounts)
        {
            std::cout << "  #" << acc->getAccountId() << " | " << acc->accountTypeName()
                      << " | Balance: " << std::fixed << std::setprecision(2) << acc->getBalance() << "\n";
        }
    }

    void handleListCustomers(BankService &service)
    {
        auto customers = service.listCustomers();
        std::cout << std::left << std::setw(6) << "ID" << std::setw(20) << "Name"
                  << std::setw(25) << "Email" << "Phone\n";
        std::cout << std::string(70, '-') << "\n";
        for (const auto &c : customers)
        {
            std::cout << std::left << std::setw(6) << c.getId() << std::setw(20) << c.getName()
                      << std::setw(25) << c.getEmail() << c.getPhone() << "\n";
        }
    }

    void handleCloseAccount(BankService &service)
    {
        int accountId = readInt("Account ID to close: ");
        if (service.closeAccount(accountId))
        {
            std::cout << "Account closed successfully.\n";
        }
        else
        {
            std::cout << "Account not found or could not be closed.\n";
        }
    }

} // namespace

int main()
{
    // --- Database ---
    try
    {
        DatabaseConfig config = loadDatabaseConfig();

        DatabaseManager::initialize(
            config.host,
            config.user,
            config.password,
            config.dbName,
            config.port);

        DatabaseManager::getInstance().ensureSchema();
    }
    catch (const DatabaseException &e)
    {
        std::cerr << "Fatal: could not connect to the database.\n"
                  << e.what() << "\n"
                  << "Check that MySQL is running and sql/schema.sql has been applied,\n"
                  << "or check the credentials in .env.\n";
        return 1;
    }

    BankService service;
    bool running = true;

    while (running)
    {
        printMenu();
        int choice = readInt("");

        // Every menu action is wrapped in the same try/catch so that any
        // BankException (or subclass) raised deep in BankService /
        // DatabaseManager is reported cleanly without terminating the
        // program -- the loop simply re-displays the menu.
        try
        {
            switch (choice)
            {
            case 1:
                handleRegisterCustomer(service);
                break;
            case 2:
                handleOpenAccount(service);
                break;
            case 3:
                handleDeposit(service);
                break;
            case 4:
                handleWithdraw(service);
                break;
            case 5:
                handleTransfer(service);
                break;
            case 6:
                handleCheckBalance(service);
                break;
            case 7:
                handleTransactionHistory(service);
                break;
            case 8:
                handleViewCustomer(service);
                break;
            case 9:
                handleListCustomers(service);
                break;
            case 10:
                handleCloseAccount(service);
                break;
            case 0:
                running = false;
                std::cout << "Thank you for using the Bank Management System.\n";
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
            }
        }
        catch (const BankException &e)
        {
            std::cout << "\n[ERROR] " << e.what() << "\n";
        }
        catch (const std::exception &e)
        {
            std::cout << "\n[UNEXPECTED ERROR] " << e.what() << "\n";
        }
    }

    return 0;
}
