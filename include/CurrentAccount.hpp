#pragma once
#include "Account.hpp"

// ============================================================================
// CurrentAccount.hpp
// Concrete Account: no minimum balance requirement, but allows withdrawals
// up to an overdraft limit, and does not accrue interest. Produced by
// AccountFactory.
// ============================================================================

class CurrentAccount : public Account {
private:
    static constexpr double OVERDRAFT_LIMIT = 10000.0;

public:
    CurrentAccount(int accountId, int customerId, double balance, std::string pinHash)
        : Account(accountId, customerId, balance, std::move(pinHash), AccountType::CURRENT) {}

    void applyMinimumBalanceRule(double withdrawalAmount) const override;
    double calculateInterest() const override; // always 0 for current accounts
    std::string accountTypeName() const override { return "Current"; }
};
