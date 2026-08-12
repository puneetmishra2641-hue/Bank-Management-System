#pragma once
#include <string>
#include "Account.hpp"

// ============================================================================
// AuthManager.hpp
// Dedicated module for PIN-based authentication, kept separate from
// BankService so authentication policy (hashing, retry limits, lockouts)
// can evolve independently of core banking operations.
// ============================================================================

class AuthManager {
public:
    // Throws AuthenticationException if the PIN is incorrect.
    static void authenticate(const Account& account, const std::string& pinAttempt);

    // Allows up to `maxAttempts` tries from console input before giving up;
    // returns true if authenticated, false if attempts are exhausted.
    static bool authenticateWithRetries(const Account& account, int maxAttempts = 3);
};
