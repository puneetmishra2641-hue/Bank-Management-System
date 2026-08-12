#pragma once
#include "Account.hpp"

// ============================================================================
// SavingsAccount.hpp
// Concrete Account: enforces a minimum balance and accrues interest at a
// fixed annual rate. Produced by AccountFactory.
// ============================================================================

class SavingsAccount : public Account {
private:
    static constexpr double MIN_BALANCE = 500.0;
    static constexpr double INTEREST_RATE = 0.04; // 4% annual

public:
    SavingsAccount(int accountId, int customerId, double balance, std::string pinHash)
        : Account(accountId, customerId, balance, std::move(pinHash), AccountType::SAVINGS) {}

    void applyMinimumBalanceRule(double withdrawalAmount) const override;
    double calculateInterest() const override;
    std::string accountTypeName() const override { return "Savings"; }
};
