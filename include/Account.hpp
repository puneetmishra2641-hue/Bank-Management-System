#pragma once
#include <string>

// ============================================================================
// Account.hpp
// Abstract base class for all account types. Declares the interface shared
// by every account (balance mutation, interest policy, PIN, metadata) and
// leaves account-type-specific business rules (minimum balance, interest
// rate, overdraft limit) to derived classes. This is the polymorphic root
// created via AccountFactory and stored as std::unique_ptr<Account>.
// ============================================================================

enum class AccountType { SAVINGS, CURRENT };

class Account {
protected:
    int accountId;
    int customerId;
    double balance;
    std::string pinHash;
    AccountType type;

public:
    Account(int accountId, int customerId, double balance,
            std::string pinHash, AccountType type)
        : accountId(accountId),
          customerId(customerId),
          balance(balance),
          pinHash(std::move(pinHash)),
          type(type) {}

    // Polymorphic base class -> virtual destructor is mandatory since
    // Account is always managed through a base-class pointer
    // (std::unique_ptr<Account>) in AccountFactory / BankService.
    virtual ~Account() = default;

    // --- Core interface implemented differently per account type ---
    virtual void applyMinimumBalanceRule(double withdrawalAmount) const = 0;
    virtual double calculateInterest() const = 0;
    virtual std::string accountTypeName() const = 0;

    // --- Shared behavior common to every account type ---
    virtual void deposit(double amount);
    virtual void withdraw(double amount);

    // --- Accessors ---
    int getAccountId() const { return accountId; }
    int getCustomerId() const { return customerId; }
    double getBalance() const { return balance; }
    const std::string& getPinHash() const { return pinHash; }
    AccountType getType() const { return type; }

    // --- Mutators used by DatabaseManager when loading from storage ---
    void setAccountId(int id) { accountId = id; }
    void setBalanceDirect(double b) { balance = b; } // used only for DB hydration

    bool verifyPin(const std::string& hashedAttempt) const {
        return pinHash == hashedAttempt;
    }
};
