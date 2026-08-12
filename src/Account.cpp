#include "Account.hpp"
#include "Exceptions.hpp"

// ============================================================================
// Account.cpp
// Implements the behavior shared by every account type. Type-specific rules
// (minimum balance, overdraft limit) are injected via the pure virtual
// applyMinimumBalanceRule() hook, following the Template Method pattern.
// ============================================================================

void Account::deposit(double amount) {
    if (amount <= 0.0) {
        throw InvalidAmountException("Deposit amount must be positive.");
    }
    balance += amount;
}

void Account::withdraw(double amount) {
    if (amount <= 0.0) {
        throw InvalidAmountException("Withdrawal amount must be positive.");
    }
    // Delegates to the derived class's specific minimum-balance / overdraft
    // policy; throws InsufficientFundsException if the rule is violated.
    applyMinimumBalanceRule(amount);
    balance -= amount;
}
