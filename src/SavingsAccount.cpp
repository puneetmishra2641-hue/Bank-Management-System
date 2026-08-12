#include "SavingsAccount.hpp"
#include "Exceptions.hpp"

void SavingsAccount::applyMinimumBalanceRule(double withdrawalAmount) const {
    if (balance - withdrawalAmount < MIN_BALANCE) {
        throw InsufficientFundsException(
            "Savings account balance cannot fall below minimum balance of " +
            std::to_string(MIN_BALANCE));
    }
}

double SavingsAccount::calculateInterest() const {
    return balance * INTEREST_RATE;
}
