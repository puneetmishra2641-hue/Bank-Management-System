#include "DatabaseManager.hpp"
#include "AccountFactory.hpp"
#include "Exceptions.hpp"
#include <cstring>
#include <stdexcept>

std::unique_ptr<DatabaseManager> DatabaseManager::instance = nullptr;
std::mutex DatabaseManager::initMutex;

// ----------------------------------------------------------------------------
// Construction / Singleton access
// ----------------------------------------------------------------------------

DatabaseManager::DatabaseManager(const std::string& host, const std::string& user,
                                  const std::string& password, const std::string& dbName,
                                  unsigned int port) {
    MYSQL* raw = mysql_init(nullptr);
    if (!raw) {
        throw DatabaseException("mysql_init() failed - out of memory.");
    }

    // Attempt to connect; the underlying database (dbName) is created
    // separately if it doesn't exist (see main.cpp bootstrap step), so we
    // connect without selecting a schema first is NOT done here -- we
    // require the schema to already exist and select it directly.
    MYSQL* connected = mysql_real_connect(
        raw, host.c_str(), user.c_str(), password.c_str(),
        dbName.c_str(), port, nullptr, 0);

    if (!connected) {
        std::string err = mysql_error(raw);
        mysql_close(raw);
        throw DatabaseException("Connection failed: " + err);
    }

    connection.reset(raw); // ownership transferred to RAII wrapper
}

void DatabaseManager::initialize(const std::string& host, const std::string& user,
                                  const std::string& password, const std::string& dbName,
                                  unsigned int port) {
    std::lock_guard<std::mutex> lock(initMutex);
    if (instance) {
        return; // already initialized - Singleton guarantees a single connection
    }
    // Cannot use std::make_unique with a private constructor; use `new`
    // directly and immediately hand ownership to the unique_ptr.
    instance.reset(new DatabaseManager(host, user, password, dbName, port));
}

DatabaseManager& DatabaseManager::getInstance() {
    if (!instance) {
        throw DatabaseException("DatabaseManager::initialize() must be called before getInstance().");
    }
    return *instance;
}

void DatabaseManager::executeUpdate(const std::string& sql) {
    if (mysql_query(connection.get(), sql.c_str()) != 0) {
        throw DatabaseException(mysql_error(connection.get()));
    }
}

// ----------------------------------------------------------------------------
// Schema bootstrap (idempotent - safe to call on every startup)
// ----------------------------------------------------------------------------

void DatabaseManager::ensureSchema() {
    executeUpdate(
        "CREATE TABLE IF NOT EXISTS customers ("
        "  customer_id INT AUTO_INCREMENT PRIMARY KEY,"
        "  name VARCHAR(100) NOT NULL,"
        "  email VARCHAR(100) NOT NULL UNIQUE,"
        "  phone VARCHAR(20) NOT NULL,"
        "  address VARCHAR(255)"
        ") ENGINE=InnoDB;"
    );

    executeUpdate(
        "CREATE TABLE IF NOT EXISTS accounts ("
        "  account_id INT AUTO_INCREMENT PRIMARY KEY,"
        "  customer_id INT NOT NULL,"
        "  account_type VARCHAR(20) NOT NULL,"
        "  balance DECIMAL(15,2) NOT NULL DEFAULT 0.00,"
        "  pin_hash VARCHAR(64) NOT NULL,"
        "  FOREIGN KEY (customer_id) REFERENCES customers(customer_id) ON DELETE CASCADE"
        ") ENGINE=InnoDB;"
    );

    executeUpdate(
        "CREATE TABLE IF NOT EXISTS transactions ("
        "  transaction_id INT AUTO_INCREMENT PRIMARY KEY,"
        "  account_id INT NOT NULL,"
        "  type VARCHAR(20) NOT NULL,"
        "  amount DECIMAL(15,2) NOT NULL,"
        "  balance_after DECIMAL(15,2) NOT NULL,"
        "  txn_timestamp VARCHAR(25) NOT NULL,"
        "  remarks VARCHAR(255),"
        "  FOREIGN KEY (account_id) REFERENCES accounts(account_id) ON DELETE CASCADE"
        ") ENGINE=InnoDB;"
    );
}

// ----------------------------------------------------------------------------
// Customer CRUD
// ----------------------------------------------------------------------------

int DatabaseManager::createCustomer(const Customer& customer) {
    const char* sql =
        "INSERT INTO customers (name, email, phone, address) VALUES (?, ?, ?, ?)";

    MYSQL_STMT* stmt = mysql_stmt_init(connection.get());
    if (!stmt) throw DatabaseException("mysql_stmt_init() failed.");

    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql))) != 0) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DatabaseException(err);
    }

    MYSQL_BIND bind[4]{};
    auto bindStr = [](MYSQL_BIND& b, const std::string& s) {
        b.buffer_type = MYSQL_TYPE_STRING;
        b.buffer = const_cast<char*>(s.c_str());
        b.buffer_length = static_cast<unsigned long>(s.size());
    };
    bindStr(bind[0], customer.getName());
    bindStr(bind[1], customer.getEmail());
    bindStr(bind[2], customer.getPhone());
    bindStr(bind[3], customer.getAddress());

    if (mysql_stmt_bind_param(stmt, bind) != 0 || mysql_stmt_execute(stmt) != 0) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        // MySQL error 1062 = duplicate entry (unique email constraint)
        if (err.find("Duplicate entry") != std::string::npos) {
            throw DuplicateEntityException("A customer with this email already exists.");
        }
        throw DatabaseException(err);
    }

    int newId = static_cast<int>(mysql_stmt_insert_id(stmt));
    mysql_stmt_close(stmt);
    return newId;
}

Customer DatabaseManager::readCustomer(int customerId) {
    const char* sql = "SELECT customer_id, name, email, phone, address FROM customers WHERE customer_id = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(connection.get());
    if (!stmt) throw DatabaseException("mysql_stmt_init() failed.");
    mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql)));

    MYSQL_BIND paramBind[1]{};
    paramBind[0].buffer_type = MYSQL_TYPE_LONG;
    paramBind[0].buffer = &customerId;
    mysql_stmt_bind_param(stmt, paramBind);

    if (mysql_stmt_execute(stmt) != 0) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DatabaseException(err);
    }

    char nameBuf[101]{}, emailBuf[101]{}, phoneBuf[21]{}, addrBuf[256]{};
    int idOut = 0;
    unsigned long nameLen = 0, emailLen = 0, phoneLen = 0, addrLen = 0;

    MYSQL_BIND resultBind[5]{};
    resultBind[0].buffer_type = MYSQL_TYPE_LONG;
    resultBind[0].buffer = &idOut;
    resultBind[1].buffer_type = MYSQL_TYPE_STRING;
    resultBind[1].buffer = nameBuf; resultBind[1].buffer_length = sizeof(nameBuf); resultBind[1].length = &nameLen;
    resultBind[2].buffer_type = MYSQL_TYPE_STRING;
    resultBind[2].buffer = emailBuf; resultBind[2].buffer_length = sizeof(emailBuf); resultBind[2].length = &emailLen;
    resultBind[3].buffer_type = MYSQL_TYPE_STRING;
    resultBind[3].buffer = phoneBuf; resultBind[3].buffer_length = sizeof(phoneBuf); resultBind[3].length = &phoneLen;
    resultBind[4].buffer_type = MYSQL_TYPE_STRING;
    resultBind[4].buffer = addrBuf; resultBind[4].buffer_length = sizeof(addrBuf); resultBind[4].length = &addrLen;

    mysql_stmt_bind_result(stmt, resultBind);

    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw InvalidAccountException("No customer found with ID " + std::to_string(customerId));
    }

    mysql_stmt_close(stmt);
    return Customer(idOut, std::string(nameBuf, nameLen), std::string(emailBuf, emailLen),
                     std::string(phoneBuf, phoneLen), std::string(addrBuf, addrLen));
}

std::vector<Customer> DatabaseManager::readAllCustomers() {
    std::vector<Customer> results;
    const char* sql = "SELECT customer_id, name, email, phone, address FROM customers ORDER BY customer_id";

    if (mysql_query(connection.get(), sql) != 0) {
        throw DatabaseException(mysql_error(connection.get()));
    }

    MYSQL_RES* res = mysql_store_result(connection.get());
    if (!res) throw DatabaseException(mysql_error(connection.get()));

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        results.emplace_back(
            std::stoi(row[0]),
            row[1] ? row[1] : "",
            row[2] ? row[2] : "",
            row[3] ? row[3] : "",
            row[4] ? row[4] : "");
    }
    mysql_free_result(res);
    return results;
}

bool DatabaseManager::updateCustomer(const Customer& customer) {
    const char* sql = "UPDATE customers SET name=?, email=?, phone=?, address=? WHERE customer_id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(connection.get());
    mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql)));

    MYSQL_BIND bind[5]{};
    auto bindStr = [](MYSQL_BIND& b, const std::string& s) {
        b.buffer_type = MYSQL_TYPE_STRING;
        b.buffer = const_cast<char*>(s.c_str());
        b.buffer_length = static_cast<unsigned long>(s.size());
    };
    bindStr(bind[0], customer.getName());
    bindStr(bind[1], customer.getEmail());
    bindStr(bind[2], customer.getPhone());
    bindStr(bind[3], customer.getAddress());
    int id = customer.getId();
    bind[4].buffer_type = MYSQL_TYPE_LONG;
    bind[4].buffer = &id;

    mysql_stmt_bind_param(stmt, bind);
    bool ok = (mysql_stmt_execute(stmt) == 0);
    my_ulonglong affected = mysql_stmt_affected_rows(stmt);
    mysql_stmt_close(stmt);
    return ok && affected > 0;
}

bool DatabaseManager::deleteCustomer(int customerId) {
    const char* sql = "DELETE FROM customers WHERE customer_id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(connection.get());
    mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql)));

    MYSQL_BIND bind[1]{};
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &customerId;
    mysql_stmt_bind_param(stmt, bind);

    bool ok = (mysql_stmt_execute(stmt) == 0);
    my_ulonglong affected = mysql_stmt_affected_rows(stmt);
    mysql_stmt_close(stmt);
    return ok && affected > 0;
}

// ----------------------------------------------------------------------------
// Account CRUD
// ----------------------------------------------------------------------------

int DatabaseManager::createAccount(const Account& account) {
    const char* sql =
        "INSERT INTO accounts (customer_id, account_type, balance, pin_hash) VALUES (?, ?, ?, ?)";
    MYSQL_STMT* stmt = mysql_stmt_init(connection.get());
    mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql)));

    int custId = account.getCustomerId();
    std::string typeStr = account.accountTypeName();
    double balance = account.getBalance();
    std::string pinHash = account.getPinHash();

    MYSQL_BIND bind[4]{};
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &custId;
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = const_cast<char*>(typeStr.c_str());
    bind[1].buffer_length = static_cast<unsigned long>(typeStr.size());
    bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[2].buffer = &balance;
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = const_cast<char*>(pinHash.c_str());
    bind[3].buffer_length = static_cast<unsigned long>(pinHash.size());

    mysql_stmt_bind_param(stmt, bind);
    if (mysql_stmt_execute(stmt) != 0) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DatabaseException(err);
    }
    int newId = static_cast<int>(mysql_stmt_insert_id(stmt));
    mysql_stmt_close(stmt);
    return newId;
}

std::unique_ptr<Account> DatabaseManager::readAccount(int accountId) {
    const char* sql = "SELECT account_id, customer_id, account_type, balance, pin_hash FROM accounts WHERE account_id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(connection.get());
    mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql)));

    MYSQL_BIND paramBind[1]{};
    paramBind[0].buffer_type = MYSQL_TYPE_LONG;
    paramBind[0].buffer = &accountId;
    mysql_stmt_bind_param(stmt, paramBind);

    if (mysql_stmt_execute(stmt) != 0) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DatabaseException(err);
    }

    int idOut = 0, custIdOut = 0;
    char typeBuf[21]{}, pinBuf[65]{};
    double balanceOut = 0.0;
    unsigned long typeLen = 0, pinLen = 0;

    MYSQL_BIND resultBind[5]{};
    resultBind[0].buffer_type = MYSQL_TYPE_LONG; resultBind[0].buffer = &idOut;
    resultBind[1].buffer_type = MYSQL_TYPE_LONG; resultBind[1].buffer = &custIdOut;
    resultBind[2].buffer_type = MYSQL_TYPE_STRING; resultBind[2].buffer = typeBuf;
    resultBind[2].buffer_length = sizeof(typeBuf); resultBind[2].length = &typeLen;
    resultBind[3].buffer_type = MYSQL_TYPE_DOUBLE; resultBind[3].buffer = &balanceOut;
    resultBind[4].buffer_type = MYSQL_TYPE_STRING; resultBind[4].buffer = pinBuf;
    resultBind[4].buffer_length = sizeof(pinBuf); resultBind[4].length = &pinLen;

    mysql_stmt_bind_result(stmt, resultBind);

    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        throw InvalidAccountException("No account found with ID " + std::to_string(accountId));
    }
    mysql_stmt_close(stmt);

    return AccountFactory::createAccount(std::string(typeBuf, typeLen), idOut, custIdOut,
                                          balanceOut, std::string(pinBuf, pinLen));
}

std::vector<std::unique_ptr<Account>> DatabaseManager::readAccountsForCustomer(int customerId) {
    std::vector<std::unique_ptr<Account>> results;
    const char* sql = "SELECT account_id FROM accounts WHERE customer_id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(connection.get());
    mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql)));

    MYSQL_BIND paramBind[1]{};
    paramBind[0].buffer_type = MYSQL_TYPE_LONG;
    paramBind[0].buffer = &customerId;
    mysql_stmt_bind_param(stmt, paramBind);
    mysql_stmt_execute(stmt);

    int idOut = 0;
    MYSQL_BIND resultBind[1]{};
    resultBind[0].buffer_type = MYSQL_TYPE_LONG;
    resultBind[0].buffer = &idOut;
    mysql_stmt_bind_result(stmt, resultBind);

    std::vector<int> ids;
    while (mysql_stmt_fetch(stmt) == 0) {
        ids.push_back(idOut);
    }
    mysql_stmt_close(stmt);

    // Reuse readAccount() (already a prepared-statement round trip) for each
    // id -- keeps the account-hydration/factory logic in one place.
    for (int id : ids) {
        results.push_back(readAccount(id));
    }
    return results;
}

bool DatabaseManager::updateAccountBalance(int accountId, double newBalance) {
    const char* sql = "UPDATE accounts SET balance=? WHERE account_id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(connection.get());
    mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql)));

    MYSQL_BIND bind[2]{};
    bind[0].buffer_type = MYSQL_TYPE_DOUBLE;
    bind[0].buffer = &newBalance;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &accountId;

    mysql_stmt_bind_param(stmt, bind);
    bool ok = (mysql_stmt_execute(stmt) == 0);
    my_ulonglong affected = mysql_stmt_affected_rows(stmt);
    mysql_stmt_close(stmt);
    return ok && affected > 0;
}

bool DatabaseManager::deleteAccount(int accountId) {
    const char* sql = "DELETE FROM accounts WHERE account_id=?";
    MYSQL_STMT* stmt = mysql_stmt_init(connection.get());
    mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql)));

    MYSQL_BIND bind[1]{};
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &accountId;
    mysql_stmt_bind_param(stmt, bind);

    bool ok = (mysql_stmt_execute(stmt) == 0);
    my_ulonglong affected = mysql_stmt_affected_rows(stmt);
    mysql_stmt_close(stmt);
    return ok && affected > 0;
}

// ----------------------------------------------------------------------------
// Transaction CRUD (append-only ledger: create + read)
// ----------------------------------------------------------------------------

int DatabaseManager::recordTransaction(const Transaction& transaction) {
    const char* sql =
        "INSERT INTO transactions (account_id, type, amount, balance_after, txn_timestamp, remarks) "
        "VALUES (?, ?, ?, ?, ?, ?)";
    MYSQL_STMT* stmt = mysql_stmt_init(connection.get());
    mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql)));

    int accId = transaction.getAccountId();
    std::string typeStr = Transaction::typeToString(transaction.getType());
    double amount = transaction.getAmount();
    double balanceAfter = transaction.getBalanceAfter();
    std::string timestamp = transaction.getTimestamp();
    std::string remarks = transaction.getRemarks();

    MYSQL_BIND bind[6]{};
    bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &accId;
    bind[1].buffer_type = MYSQL_TYPE_STRING; bind[1].buffer = const_cast<char*>(typeStr.c_str());
    bind[1].buffer_length = static_cast<unsigned long>(typeStr.size());
    bind[2].buffer_type = MYSQL_TYPE_DOUBLE; bind[2].buffer = &amount;
    bind[3].buffer_type = MYSQL_TYPE_DOUBLE; bind[3].buffer = &balanceAfter;
    bind[4].buffer_type = MYSQL_TYPE_STRING; bind[4].buffer = const_cast<char*>(timestamp.c_str());
    bind[4].buffer_length = static_cast<unsigned long>(timestamp.size());
    bind[5].buffer_type = MYSQL_TYPE_STRING; bind[5].buffer = const_cast<char*>(remarks.c_str());
    bind[5].buffer_length = static_cast<unsigned long>(remarks.size());

    mysql_stmt_bind_param(stmt, bind);
    if (mysql_stmt_execute(stmt) != 0) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DatabaseException(err);
    }
    int newId = static_cast<int>(mysql_stmt_insert_id(stmt));
    mysql_stmt_close(stmt);
    return newId;
}

std::vector<Transaction> DatabaseManager::readTransactionsForAccount(int accountId) {
    std::vector<Transaction> results;
    const char* sql =
        "SELECT transaction_id, account_id, type, amount, balance_after, txn_timestamp, remarks "
        "FROM transactions WHERE account_id=? ORDER BY transaction_id DESC";
    MYSQL_STMT* stmt = mysql_stmt_init(connection.get());
    mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql)));

    MYSQL_BIND paramBind[1]{};
    paramBind[0].buffer_type = MYSQL_TYPE_LONG;
    paramBind[0].buffer = &accountId;
    mysql_stmt_bind_param(stmt, paramBind);
    mysql_stmt_execute(stmt);

    int txnId = 0, accIdOut = 0;
    char typeBuf[21]{}, tsBuf[26]{}, remarksBuf[256]{};
    double amount = 0.0, balanceAfter = 0.0;
    unsigned long typeLen = 0, tsLen = 0, remarksLen = 0;

    MYSQL_BIND resultBind[7]{};
    resultBind[0].buffer_type = MYSQL_TYPE_LONG; resultBind[0].buffer = &txnId;
    resultBind[1].buffer_type = MYSQL_TYPE_LONG; resultBind[1].buffer = &accIdOut;
    resultBind[2].buffer_type = MYSQL_TYPE_STRING; resultBind[2].buffer = typeBuf;
    resultBind[2].buffer_length = sizeof(typeBuf); resultBind[2].length = &typeLen;
    resultBind[3].buffer_type = MYSQL_TYPE_DOUBLE; resultBind[3].buffer = &amount;
    resultBind[4].buffer_type = MYSQL_TYPE_DOUBLE; resultBind[4].buffer = &balanceAfter;
    resultBind[5].buffer_type = MYSQL_TYPE_STRING; resultBind[5].buffer = tsBuf;
    resultBind[5].buffer_length = sizeof(tsBuf); resultBind[5].length = &tsLen;
    resultBind[6].buffer_type = MYSQL_TYPE_STRING; resultBind[6].buffer = remarksBuf;
    resultBind[6].buffer_length = sizeof(remarksBuf); resultBind[6].length = &remarksLen;

    mysql_stmt_bind_result(stmt, resultBind);

    while (mysql_stmt_fetch(stmt) == 0) {
        results.emplace_back(
            txnId, accIdOut, Transaction::stringToType(std::string(typeBuf, typeLen)),
            amount, balanceAfter, std::string(tsBuf, tsLen), std::string(remarksBuf, remarksLen));
    }
    mysql_stmt_close(stmt);
    return results;
}
