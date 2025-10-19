#pragma once
#ifndef SECUREDATABASE_H
#define SECUREDATABASE_H

#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <cassert>

// Simple user structure
struct User {
    std::string username;
    std::string password; // stored encrypted
    std::string role;     // e.g., "admin", "user"
};

// RAII wrapper for database connection (simulated)
class DBConnection {
public:
    DBConnection();
    ~DBConnection() noexcept;
    bool execute(const std::string& query, const std::vector<std::string>& params);
};

class SecureDatabase {
public:
    SecureDatabase();
    ~SecureDatabase() noexcept;

    // CRUD operations
    bool AddUser(const User& user, const std::string& currentRole);
    std::unique_ptr<User> GetUser(const std::string& username, const std::string& currentRole);
    bool UpdatePassword(const std::string& username, const std::string& newPassword, const std::string& currentRole);
    bool DeleteUser(const std::string& username, const std::string& currentRole);

private:
    std::unique_ptr<DBConnection> db;

    bool Authorized(const std::string& role, const std::string& operation);
    std::string Encrypt(const std::string& plainText);
    void Log(const std::string& message);
};

#endif // SECUREDATABASE_H
