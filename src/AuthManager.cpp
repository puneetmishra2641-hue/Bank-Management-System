#include "AuthManager.hpp"
#include "Exceptions.hpp"
#include "Utils.hpp"
#include <iostream>

void AuthManager::authenticate(const Account& account, const std::string& pinAttempt) {
    std::string hashedAttempt = Utils::hashPin(pinAttempt);
    if (!account.verifyPin(hashedAttempt)) {
        throw AuthenticationException("Incorrect PIN for account #" +
                                       std::to_string(account.getAccountId()));
    }
}

bool AuthManager::authenticateWithRetries(const Account& account, int maxAttempts) {
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        std::string pin;
        std::cout << "Enter PIN (attempt " << attempt << "/" << maxAttempts << "): ";
        std::cin >> pin;
        try {
            authenticate(account, pin);
            return true;
        } catch (const AuthenticationException& e) {
            std::cout << e.what() << "\n";
        }
    }
    return false;
}
