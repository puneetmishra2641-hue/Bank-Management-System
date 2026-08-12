#pragma once
#include <memory>
#include <string>
#include "Account.hpp"

// ============================================================================
// AccountFactory.hpp
// Factory Method pattern: centralizes object creation for the Account
// hierarchy so callers (BankService, DatabaseManager) never need to know
// about SavingsAccount / CurrentAccount concrete types directly. Returns
// ownership via std::unique_ptr<Account> to keep memory management RAII-safe
// and exception-safe (no raw new/delete, no leaks on throw).
// ============================================================================

class AccountFactory {
public:
    static std::unique_ptr<Account> createAccount(
        AccountType type,
        int accountId,
        int customerId,
        double initialBalance,
        const std::string& pinHash);

    // Convenience overload that parses the type from a string, used when
    // reading account_type back out of the database or console input.
    static std::unique_ptr<Account> createAccount(
        const std::string& typeStr,
        int accountId,
        int customerId,
        double initialBalance,
        const std::string& pinHash);

    static AccountType parseType(const std::string& typeStr);
    static std::string typeToString(AccountType type);
};
