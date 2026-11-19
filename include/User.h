#ifndef USER_H
#define USER_H

#include <string>
#include <list>
#include <fstream>
#include <sstream>
#include <functional>


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

    std::list<std::string> splitCSV(const std::string& line) const {
        std::list<std::string> parts;
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
            if (!parts.empty() && parts.front() == username) return true;
        }
        return false;
    }

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

    bool signIn(const std::string& username, const std::string& password, User& outUser) const {
        std::ifstream in(dbPath);
        if (!in.is_open()) return false;
        std::string line;
        size_t pHash = hasher(password);
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            auto parts = splitCSV(line);
            if (parts.size() >= 3 && parts.front() == username) {
                auto field = parts.begin();
                size_t storedHash = std::stoull(*(++field));   
                if (storedHash == pHash) {
                    outUser = User(username, storedHash, std::stod(*(++field))); 
                    return true;
                }
                return false;
            }
        }
        return false;
    }

    bool saveBalance(const User& user) {
        std::ifstream in(dbPath);
        if (!in.is_open()) return false;
        std::list<std::string> lines;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            auto parts = splitCSV(line);
            if (!parts.empty() && parts.front() == user.getUsername()) {
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

#endif 
