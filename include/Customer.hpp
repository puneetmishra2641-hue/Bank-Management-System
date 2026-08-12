#pragma once
#include <string>
#include <iostream>

// ============================================================================
// Customer.hpp
// Plain data/entity class representing a bank customer. Kept independent of
// Account/DatabaseManager so it can be reused (e.g. serialized to JSON,
// mapped to a different persistence layer) without pulling in the rest of
// the system.
// ============================================================================

class Customer {
private:
    int customerId;
    std::string name;
    std::string email;
    std::string phone;
    std::string address;

public:
    Customer() : customerId(-1) {}

    Customer(int id, std::string name, std::string email,
             std::string phone, std::string address)
        : customerId(id),
          name(std::move(name)),
          email(std::move(email)),
          phone(std::move(phone)),
          address(std::move(address)) {}

    // --- Accessors ---
    int getId() const { return customerId; }
    const std::string& getName() const { return name; }
    const std::string& getEmail() const { return email; }
    const std::string& getPhone() const { return phone; }
    const std::string& getAddress() const { return address; }

    // --- Mutators ---
    void setId(int id) { customerId = id; }
    void setName(const std::string& n) { name = n; }
    void setEmail(const std::string& e) { email = e; }
    void setPhone(const std::string& p) { phone = p; }
    void setAddress(const std::string& a) { address = a; }

    void display() const {
        std::cout << "Customer ID : " << customerId << "\n"
                  << "Name        : " << name << "\n"
                  << "Email       : " << email << "\n"
                  << "Phone       : " << phone << "\n"
                  << "Address     : " << address << "\n";
    }
};
