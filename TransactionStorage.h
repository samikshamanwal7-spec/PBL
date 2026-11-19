#ifndef TRANSACTION_STORAGE_H
#define TRANSACTION_STORAGE_H

#include <string>
#include <list>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <cctype>
#include <cstdio>
#include <ctime>
#include "modules/trading_portfolio/Portfolio.h"

// ============================================================
// TransactionStorage.h -- Transactions & Orders module (file handling)
//
// Saves and restores a user's holdings, realized P&L and full
// transaction history so they survive between sessions.
// (Cash balance is stored separately in users.db by User.h.)
//
// File: data/portfolio_<username>.db -- plain text, one record per line:
//   R,<realizedPnL>
//   H,<symbol>,<quantity>,<avgBuyPrice>
//   T,<0=BUY|1=SELL>,<symbol>,<quantity>,<price>,<realizedPnL>,<timestamp>
// Transactions are written oldest-first so that loading them back
// (push in this order) rebuilds the same stack.
// ============================================================
class TransactionStorage {
public:
    // Non-alphanumeric characters in the username are hex-encoded so any
    // username maps to a safe, unique file name.
    static std::string pathFor(const std::string& username) {
        std::string safe;
        for (unsigned char c : username) {
            if (std::isalnum(c)) safe += static_cast<char>(c);
            else { char buf[4]; std::snprintf(buf, sizeof buf, "_%02X", c); safe += buf; }
        }
        return "data/portfolio_" + safe + ".db";
    }

    static bool save(const Portfolio& portfolio, const std::string& path) {
        std::ofstream out(path, std::ios::trunc);
        if (!out.is_open()) return false;
        out << std::setprecision(12);
        out << "R," << portfolio.getRealizedPnL() << "\n";
        for (const auto& [symbol, h] : portfolio.allHoldings()) {
            if (h.quantity > 0)
                out << "H," << symbol << "," << h.quantity << "," << h.avgBuyPrice << "\n";
        }
        for (const Transaction& t : portfolio.getHistory().oldestFirst()) {
            out << "T," << (t.type == TxType::BUY ? 0 : 1) << "," << t.symbol << ","
                << t.quantity << "," << t.priceEach << "," << t.realizedPnL << ","
                << static_cast<long long>(t.timestamp) << "\n";
        }
        return true;
    }

    // A missing file is fine (new user): the portfolio simply stays empty.
    // Returns true if a file was read.
    static bool load(Portfolio& portfolio, const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) return false;

        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::list<std::string> f;
            std::stringstream ss(line);
            std::string part;
            while (std::getline(ss, part, ',')) f.push_back(part);

            try {
                if (f.front() == "R" && f.size() >= 2) {
                    portfolio.restoreRealizedPnL(std::stod(*std::next(f.begin())));
                } else if (f.front() == "H" && f.size() >= 4) {
                    auto it = std::next(f.begin());
                    std::string symbol = *it++;
                    int qty = std::stoi(*it++);
                    double avg = std::stod(*it);
                    portfolio.restoreHolding(symbol, qty, avg);
                } else if (f.front() == "T" && f.size() >= 7) {
                    auto it = std::next(f.begin());
                    Transaction t;
                    t.type        = (std::stoi(*it++) == 0) ? TxType::BUY : TxType::SELL;
                    t.symbol      = *it++;
                    t.quantity    = std::stoi(*it++);
                    t.priceEach   = std::stod(*it++);
                    t.realizedPnL = std::stod(*it++);
                    t.timestamp   = static_cast<std::time_t>(std::stoll(*it));
                    portfolio.restoreTransaction(t);
                }
            } catch (...) {
                // Skip a corrupt line rather than crash.
            }
        }
        return true;
    }
};

#endif // TRANSACTION_STORAGE_H
