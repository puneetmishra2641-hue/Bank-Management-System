#pragma once
#include <string>

// ============================================================================
// Transaction.hpp
// Immutable record of a single ledger event (deposit, withdrawal, transfer
// leg). Persisted to the `transactions` table and used to reconstruct
// per-account transaction history.
// ============================================================================

enum class TransactionType { DEPOSIT, WITHDRAWAL, TRANSFER_IN, TRANSFER_OUT };

class Transaction {
private:
    int transactionId;
    int accountId;
    TransactionType type;
    double amount;
    double balanceAfter;
    std::string timestamp;
    std::string remarks;

public:
    Transaction(int transactionId, int accountId, TransactionType type,
                double amount, double balanceAfter, std::string timestamp,
                std::string remarks = "")
        : transactionId(transactionId),
          accountId(accountId),
          type(type),
          amount(amount),
          balanceAfter(balanceAfter),
          timestamp(std::move(timestamp)),
          remarks(std::move(remarks)) {}

    int getTransactionId() const { return transactionId; }
    int getAccountId() const { return accountId; }
    TransactionType getType() const { return type; }
    double getAmount() const { return amount; }
    double getBalanceAfter() const { return balanceAfter; }
    const std::string& getTimestamp() const { return timestamp; }
    const std::string& getRemarks() const { return remarks; }

    static std::string typeToString(TransactionType t) {
        switch (t) {
            case TransactionType::DEPOSIT:      return "DEPOSIT";
            case TransactionType::WITHDRAWAL:   return "WITHDRAWAL";
            case TransactionType::TRANSFER_IN:  return "TRANSFER_IN";
            case TransactionType::TRANSFER_OUT: return "TRANSFER_OUT";
        }
        return "UNKNOWN";
    }

    static TransactionType stringToType(const std::string& s) {
        if (s == "DEPOSIT") return TransactionType::DEPOSIT;
        if (s == "WITHDRAWAL") return TransactionType::WITHDRAWAL;
        if (s == "TRANSFER_IN") return TransactionType::TRANSFER_IN;
        if (s == "TRANSFER_OUT") return TransactionType::TRANSFER_OUT;
        return TransactionType::DEPOSIT;
    }
};
