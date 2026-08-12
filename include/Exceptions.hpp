#pragma once
#include <stdexcept>
#include <string>

// ============================================================================
// Exceptions.hpp
// Centralized exception hierarchy for the Bank Management System.
// All custom exceptions derive from BankException so callers can catch
// broadly (catch (const BankException&)) or narrowly (catch (const
// InsufficientFundsException&)) as needed.
// ============================================================================

class BankException : public std::runtime_error {
public:
    explicit BankException(const std::string& message)
        : std::runtime_error(message) {}
};

class DatabaseException : public BankException {
public:
    explicit DatabaseException(const std::string& message)
        : BankException("Database Error: " + message) {}
};

class InvalidAccountException : public BankException {
public:
    explicit InvalidAccountException(const std::string& message)
        : BankException("Invalid Account: " + message) {}
};

class InsufficientFundsException : public BankException {
public:
    explicit InsufficientFundsException(const std::string& message)
        : BankException("Insufficient Funds: " + message) {}
};

class AuthenticationException : public BankException {
public:
    explicit AuthenticationException(const std::string& message)
        : BankException("Authentication Failed: " + message) {}
};

class InvalidAmountException : public BankException {
public:
    explicit InvalidAmountException(const std::string& message)
        : BankException("Invalid Amount: " + message) {}
};

class DuplicateEntityException : public BankException {
public:
    explicit DuplicateEntityException(const std::string& message)
        : BankException("Duplicate Entity: " + message) {}
};
