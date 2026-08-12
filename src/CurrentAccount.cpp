#include "CurrentAccount.hpp"
#include "Exceptions.hpp"

void CurrentAccount::applyMinimumBalanceRule(double withdrawalAmount) const {
    // Current accounts may dip into a negative balance up to the overdraft
    // limit rather than enforcing a strict minimum balance.
    if (balance - withdrawalAmount < -OVERDRAFT_LIMIT) {
        throw InsufficientFundsException(
            "Withdrawal exceeds overdraft limit of " +
            std::to_string(OVERDRAFT_LIMIT));
    }
}

double CurrentAccount::calculateInterest() const {
    return 0.0;
}
