# Bank Management System (C++ / MySQL Console Application)

A modular, object-oriented banking console application built in modern C++
(C++17) with a normalized MySQL backend. Built as a resume/portfolio project
demonstrating OOP design, design patterns, RAII resource management, and
database-backed persistence.

## Features

- Customer and Account management (create, read, update, delete)
- Savings and Current accounts with distinct business rules
  (minimum balance vs. overdraft limit) via polymorphism
- Deposits, withdrawals, and inter-account fund transfers
- PIN-based authentication on every sensitive operation
- Full transaction ledger / history per account
- Centralized custom exception hierarchy for robust error handling
- Persistent storage in a normalized MySQL schema (3 tables, FK-linked)

## Architecture & Design Patterns

| Concern                     | Approach                                                        |
|------------------------------|------------------------------------------------------------------|
| Object creation               | **Factory pattern** (`AccountFactory`) builds `SavingsAccount` / `CurrentAccount` behind a common `Account` interface |
| Single DB connection          | **Singleton pattern** (`DatabaseManager::getInstance()`) — one connection for the app's lifetime, guarded by a mutex at init |
| Resource safety                | **RAII** — the raw `MYSQL*` handle is wrapped in `std::unique_ptr` with a custom deleter (`MySQLConnDeleter`) so `mysql_close()` always runs, even on exceptions |
| Polymorphism                   | `Account` is an abstract base class; `deposit()`/`withdraw()` use the Template Method pattern, delegating minimum-balance/overdraft policy to derived classes |
| Memory management               | No raw `new`/`delete` for domain objects — accounts are owned via `std::unique_ptr<Account>` throughout |
| Error handling                  | A `BankException` hierarchy (`InsufficientFundsException`, `AuthenticationException`, `DatabaseException`, etc.) is thrown deep in the stack and caught once at the top of `main()` |
| Persistence abstraction         | `DatabaseManager` is the only class that includes `<mysql/mysql.h>`; everything else talks to it through a plain C++ interface |

## Project Structure

```
BankManagementSystem/
├── CMakeLists.txt
├── include/                 # Headers (interfaces)
│   ├── Account.hpp          # Abstract base class
│   ├── SavingsAccount.hpp
│   ├── CurrentAccount.hpp
│   ├── AccountFactory.hpp   # Factory pattern
│   ├── Customer.hpp
│   ├── Transaction.hpp
│   ├── DatabaseManager.hpp  # Singleton + RAII DB wrapper, all CRUD SQL
│   ├── AuthManager.hpp      # PIN authentication
│   ├── BankService.hpp      # Business-logic facade
│   ├── Exceptions.hpp       # Custom exception hierarchy
│   └── Utils.hpp
├── src/                      # Implementations
│   ├── Account.cpp
│   ├── SavingsAccount.cpp
│   ├── CurrentAccount.cpp
│   ├── AccountFactory.cpp
│   ├── DatabaseManager.cpp
│   ├── AuthManager.cpp
│   ├── BankService.cpp
│   └── main.cpp              # Console UI / entry point
└── sql/
    └── schema.sql             # Standalone DDL (also auto-applied at runtime)
```

## Database Schema

```
customers (1) ──< accounts (1) ──< transactions
```

- `customers(customer_id PK, name, email UNIQUE, phone, address)`
- `accounts(account_id PK, customer_id FK, account_type, balance, pin_hash)`
- `transactions(transaction_id PK, account_id FK, type, amount, balance_after, txn_timestamp, remarks)`

Foreign keys cascade on delete, so removing a customer cleans up their
accounts and transaction history automatically.

## Build & Run

### Prerequisites
- CMake >= 3.16, a C++17 compiler
- MySQL or MariaDB server, plus development headers:
  `sudo apt install libmysqlclient-dev cmake g++` (Ubuntu/Debian)

### 1. Set up the database
```bash
mysql -u root -p < sql/schema.sql
mysql -u root -p -e "
  CREATE USER 'bank_app'@'localhost' IDENTIFIED BY 'change_me';
  GRANT ALL PRIVILEGES ON bank_management_system.* TO 'bank_app'@'localhost';
  FLUSH PRIVILEGES;"
```
(`DatabaseManager::ensureSchema()` also creates the tables automatically at
startup if the database already exists but is empty — `sql/schema.sql` is
for manual/first-time setup and documentation.)

### 2. Update credentials
Edit the `DatabaseManager::initialize(...)` call at the top of `src/main.cpp`
if your host/user/password/port differ from the defaults.

### 3. Build
```bash
mkdir build && cd build
cmake ..
make -j4
```

### 4. Run
```bash
./bank_app
```
