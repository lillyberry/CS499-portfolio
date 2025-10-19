#include "SecureDatabase.h"

int main() {
    SecureDatabase db;

    User newUser{ "alice", "SuperSecret123!", "user" };

    // Attempt to add user as normal user (should fail)
    db.AddUser(newUser, "user");

    // Add user as admin (should succeed)
    db.AddUser(newUser, "admin");

    // Retrieve user information
    auto retrieved = db.GetUser("alice", "user");
    if (retrieved) {
        std::cout << "Retrieved user: " << retrieved->username << " Role: " << retrieved->role << std::endl;
    }

    // Update password (admin only)
    db.UpdatePassword("alice", "NewSecret456!", "admin");

    // Delete user (admin only)
    db.DeleteUser("alice", "admin");

    return 0;
}
