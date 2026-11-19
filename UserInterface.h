#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <string>
#include "common/Currency.h"
#include "common/Console.h"
#include "common/InputHelpers.h"
#include "modules/user_management/User.h"

// ============================================================
// UserInterface.h -- User Management module (interface)
// The Login / Register screen. Talks to AuthManager (User.h).
// ============================================================

// Returns true once the user is signed in, false if they chose Exit.
inline bool authFlow(AuthManager& auth, User& currentUser) {
    while (true) {
        ui::banner("Virtual Stock Market Learning Platform");
        ui::menuBox("Welcome", {"Sign In", "Sign Up", "Exit"});
        int choice = input::readInt();

        if (choice == 1) {
            std::string uname = input::readLine("Username: ");
            std::string pass  = input::readLine("Password: ");
            if (auth.signIn(uname, pass, currentUser)) {
                ui::success("Welcome back, " + uname + "!");
                ui::pause();
                return true;
            } else {
                ui::error("Invalid username or password.");
                ui::pause();
            }
        } else if (choice == 2) {
            std::string uname = input::readLine("Choose a username: ");
            std::string pass  = input::readLine("Choose a password: ");
            if (auth.signUp(uname, pass, currentUser)) {
                ui::success("Account created! Starting balance: " + currency::SYMBOL + "10000.00");
                ui::pause();
                return true;
            } else {
                ui::error("Username already exists or fields were empty.");
                ui::pause();
            }
        } else if (choice == 3) {
            return false;
        } else {
            ui::error("Invalid choice.");
            ui::pause();
        }
    }
}

#endif // USER_INTERFACE_H
