#include "AccountFactory.hpp"
#include "SavingsAccount.hpp"
#include "CurrentAccount.hpp"
#include "Exceptions.hpp"
#include <algorithm>
#include <cctype>

std::unique_ptr<Account> AccountFactory::createAccount(
    AccountType type,
    int accountId,
    int customerId,
    double initialBalance,
    const std::string& pinHash) {

    switch (type) {
        case AccountType::SAVINGS:
            return std::make_unique<SavingsAccount>(accountId, customerId, initialBalance, pinHash);
        case AccountType::CURRENT:
            return std::make_unique<CurrentAccount>(accountId, customerId, initialBalance, pinHash);
    }
    // Unreachable for a valid enum value, but keeps the compiler and
    // exception-safety contract happy if the enum is ever extended.
    throw InvalidAccountException("Unknown account type requested from factory.");
}

std::unique_ptr<Account> AccountFactory::createAccount(
    const std::string& typeStr,
    int accountId,
    int customerId,
    double initialBalance,
    const std::string& pinHash) {
    return createAccount(parseType(typeStr), accountId, customerId, initialBalance, pinHash);
}

AccountType AccountFactory::parseType(const std::string& typeStr) {
    std::string lower = typeStr;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (lower == "savings") return AccountType::SAVINGS;
    if (lower == "current") return AccountType::CURRENT;

    throw InvalidAccountException("Unrecognized account type: '" + typeStr + "'");
}

std::string AccountFactory::typeToString(AccountType type) {
    switch (type) {
        case AccountType::SAVINGS: return "Savings";
        case AccountType::CURRENT: return "Current";
    }
    return "Unknown";
}
