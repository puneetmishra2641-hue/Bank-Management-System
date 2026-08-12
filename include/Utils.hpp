#pragma once
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <functional>
#include <ctime> // Required for std::time_t, std::tm, std::localtime

// ============================================================================
// Utils.hpp
// Small stateless helper functions shared across modules: timestamp
// formatting and PIN hashing. Kept free of dependencies on other project
// headers so any module can include it without creating a cycle.
// ============================================================================

namespace Utils
{

    // Returns the current local time formatted as "YYYY-MM-DD HH:MM:SS".
    inline std::string currentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tmBuf{};
#if defined(_WIN32)
        tmBuf = *std::localtime(&t);
#else
        localtime_r(&t, &tmBuf);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    // NOTE: std::hash is used here purely to avoid storing PINs in plain
    // text in the demo database. It is NOT cryptographically secure and
    // should be replaced with a proper salted hash (e.g. bcrypt/Argon2)
    // in a production system.
    inline std::string hashPin(const std::string &pin)
    {
        std::hash<std::string> hasher;
        size_t hashed = hasher(pin);
        std::ostringstream oss;
        oss << std::hex << hashed;
        return oss.str();
    }

} // namespace Utils