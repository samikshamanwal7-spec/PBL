#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <functional>

// ============================================================
// User.h -- OOP: ENCAPSULATION
// AuthManager -- sign up / sign in against a local flat file
// (data/users.db), so the whole app works fully offline with
// no external database or network call.
//
// NOTE: password "hashing" here uses std::hash purely to avoid
// storing raw plaintext in the demo data file. This is for a
// learning project only and is NOT cryptographically secure --
// do not reuse this approach for a real authentication system.
// ============================================================

class User {
private:
    std::string username;
    size_t passwordHash;
    double cashBalance;

public:
    User() : passwordHash(0), cashBalance(0) {}
    User(std::string uname, size_t pHash, double balance)
        : username(std::move(uname)), passwordHash(pHash), cashBalance(balance) {}

    const std::string& getUsername() const { return username; }
    size_t getPasswordHash() const { return passwordHash; }
    double getCashBalance() const { return cashBalance; }
    void setCashBalance(double bal) { cashBalance = bal; }
};

class AuthManager {
private:
    std::string dbPath;
    std::hash<std::string> hasher;

    static const double STARTING_BALANCE;

    std::vector<std::string> splitCSV(const std::string& line) const {
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, ',')) parts.push_back(item);
        return parts;
    }

public:
    explicit AuthManager(std::string path) : dbPath(std::move(path)) {}

    bool userExists(const std::string& username) const {
        std::ifstream in(dbPath);
        if (!in.is_open()) return false;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            auto parts = splitCSV(line);
            if (!parts.empty() && parts[0] == username) return true;
        }
        return false;
    }

    // Returns true and fills `outUser` on success.
    bool signUp(const std::string& username, const std::string& password, User& outUser) {
        if (username.empty() || password.empty()) return false;
        if (userExists(username)) return false;

        size_t pHash = hasher(password);
        std::ofstream out(dbPath, std::ios::app);
        if (!out.is_open()) return false;
        out << username << "," << pHash << "," << STARTING_BALANCE << "\n";
        out.close();

        outUser = User(username, pHash, STARTING_BALANCE);
        return true;
    }

    // Returns true and fills `outUser` on successful login.
    bool signIn(const std::string& username, const std::string& password, User& outUser) const {
        std::ifstream in(dbPath);
        if (!in.is_open()) return false;
        std::string line;
        size_t pHash = hasher(password);
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            auto parts = splitCSV(line);
            if (parts.size() >= 3 && parts[0] == username) {
                size_t storedHash = std::stoull(parts[1]);
                if (storedHash == pHash) {
                    outUser = User(username, storedHash, std::stod(parts[2]));
                    return true;
                }
                return false; // username matched, password did not
            }
        }
        return false;
    }

    // Persists the user's updated cash balance back to the file.
    bool saveBalance(const User& user) {
        std::ifstream in(dbPath);
        if (!in.is_open()) return false;
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            auto parts = splitCSV(line);
            if (!parts.empty() && parts[0] == user.getUsername()) {
                std::ostringstream oss;
                oss << user.getUsername() << "," << user.getPasswordHash()
                    << "," << user.getCashBalance();
                lines.push_back(oss.str());
            } else {
                lines.push_back(line);
            }
        }
        in.close();

        std::ofstream out(dbPath, std::ios::trunc);
        if (!out.is_open()) return false;
        for (auto& l : lines) out << l << "\n";
        return true;
    }
};

inline const double AuthManager::STARTING_BALANCE = 10000.0;

#endif // USER_H
