#include "SecureDatabase.h"
#include <openssl/sha.h> // For simple SHA-256 encryption simulation

// --------------------- DBConnection ---------------------
DBConnection::DBConnection() {
    std::cout << "Database connection established.\n";
}

DBConnection::~DBConnection() noexcept {
    std::cout << "Database connection closed.\n";
}

bool DBConnection::execute(const std::string& query, const std::vector<std::string>& params) {
    // Simulated parameterized query execution
    std::cout << "Executing query: " << query << "\n";
    for (const auto& p : params)
        std::cout << " - param: " << p << "\n";
    return true;
}

// --------------------- SecureDatabase ---------------------
SecureDatabase::SecureDatabase() {
    db = std::make_unique<DBConnection>();
}

SecureDatabase::~SecureDatabase() noexcept {
    // RAII automatically cleans up db
}

// Simple authorization based on role
bool SecureDatabase::Authorized(const std::string& role, const std::string& operation) {
    if (role == "admin") return true;
    if (role == "user" && (operation == "SELECT")) return true;
    return false;
}

// SHA-256 encryption simulation
std::string SecureDatabase::Encrypt(const std::string& plainText) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(plainText.c_str()), plainText.size(), hash);
    std::string encrypted;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        char buf[3];
        sprintf(buf, "%02x", hash[i]);
        encrypted += buf;
    }
    return encrypted;
}

// Simple logging (masking sensitive data)
void SecureDatabase::Log(const std::string& message) {
    std::cout << "[LOG] " << message << std::endl;
}

// --------------------- CRUD ---------------------
bool SecureDatabase::AddUser(const User& user, const std::string& currentRole) {
    assert(!user.username.empty() && !user.password.empty());
    if (!Authorized(currentRole, "INSERT")) {
        Log("Unauthorized attempt to add user by role: " + currentRole);
        return false;
    }
    std::string query = "INSERT INTO Users (username, password, role) VALUES (?, ?, ?)";
    std::vector<std::string> params = { user.username, Encrypt(user.password), user.role };
    return db->execute(query, params);
}

std::unique_ptr<User> SecureDatabase::GetUser(const std::string& username, const std::string& currentRole) {
    assert(!username.empty());
    if (!Authorized(currentRole, "SELECT")) {
        Log("Unauthorized attempt to get user by role: " + currentRole);
        return nullptr;
    }
    std::string query = "SELECT username, password, role FROM Users WHERE username = ?";
    std::vector<std::string> params = { username };
    db->execute(query, params);

    // Simulated return
    auto u = std::make_unique<User>();
    u->username = username;
    u->password = "[ENCRYPTED]";
    u->role = "user";
    return u;
}

bool SecureDatabase::UpdatePassword(const std::string& username, const std::string& newPassword, const std::string& currentRole) {
    assert(!username.empty() && !newPassword.empty());
    if (!Authorized(currentRole, "UPDATE")) {
        Log("Unauthorized attempt to update password by role: " + currentRole);
        return false;
    }
    std::string query = "UPDATE Users SET password = ? WHERE username = ?";
    std::vector<std::string> params = { Encrypt(newPassword), username };
    return db->execute(query, params);
}

bool SecureDatabase::DeleteUser(const std::string& username, const std::string& currentRole) {
    assert(!username.empty());
    if (!Authorized(currentRole, "DELETE")) {
        Log("Unauthorized attempt to delete user by role: " + currentRole);
        return false;
    }
    std::string query = "DELETE FROM Users WHERE username = ?";
    std::vector<std::string> params = { username };
    return db->execute(query, params);
}
