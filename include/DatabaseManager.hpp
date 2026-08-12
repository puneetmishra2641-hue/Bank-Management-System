#pragma once
#include <mysql.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include "Customer.hpp"
#include "Account.hpp"
#include "Transaction.hpp"

// ============================================================================
// DatabaseManager.hpp
// Singleton pattern: exactly one MySQL connection is owned by the process,
// obtained via DatabaseManager::getInstance(). Copy/move are deleted to
// prevent accidental duplication of the connection handle.
//
// RAII: the raw MYSQL* connection is wrapped in a std::unique_ptr with a
// custom deleter (MySQLConnDeleter) so mysql_close() is guaranteed to run
// exactly once, even if an exception unwinds the stack -- no manual cleanup
// paths, no leaked connections.
//
// This class owns all CRUD SQL for customers, accounts, and transactions
// and is the only module that talks to libmysqlclient directly; every other
// module goes through this interface (abstraction of the storage layer).
// ============================================================================

struct MySQLConnDeleter
{
    void operator()(MYSQL *conn) const
    {
        if (conn)
            mysql_close(conn);
    }
};

class DatabaseManager
{
private:
    std::unique_ptr<MYSQL, MySQLConnDeleter> connection;
    static std::unique_ptr<DatabaseManager> instance;
    static std::mutex initMutex;

    // Private constructor enforces creation only through getInstance().
    DatabaseManager(const std::string &host, const std::string &user,
                    const std::string &password, const std::string &dbName,
                    unsigned int port);

    void executeUpdate(const std::string &sql);

public:
    // Non-copyable, non-movable: there is exactly one connection for the
    // lifetime of the application.
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;
    DatabaseManager(DatabaseManager &&) = delete;
    DatabaseManager &operator=(DatabaseManager &&) = delete;

    ~DatabaseManager() = default; // unique_ptr deleter handles mysql_close()

    // Must be called once at startup before any getInstance() calls.
    static void initialize(const std::string &host, const std::string &user,
                           const std::string &password, const std::string &dbName,
                           unsigned int port = 3306);

    static DatabaseManager &getInstance();

    void ensureSchema(); // creates tables if they do not already exist

    // ---------------- Customer CRUD ----------------
    int createCustomer(const Customer &customer);
    Customer readCustomer(int customerId);
    std::vector<Customer> readAllCustomers();
    bool updateCustomer(const Customer &customer);
    bool deleteCustomer(int customerId);

    // ---------------- Account CRUD ----------------
    int createAccount(const Account &account);
    std::unique_ptr<Account> readAccount(int accountId);
    std::vector<std::unique_ptr<Account>> readAccountsForCustomer(int customerId);
    bool updateAccountBalance(int accountId, double newBalance);
    bool deleteAccount(int accountId);

    // ---------------- Transaction CRUD (create + read; ledger is append-only) ----------------
    int recordTransaction(const Transaction &transaction);
    std::vector<Transaction> readTransactionsForAccount(int accountId);
};
