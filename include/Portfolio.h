#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include <string>
#include <unordered_map>
#include <list>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <cmath>
#include <fstream>
#include "TransactionStack.h"
#include "Currency.h"

namespace detail {
    inline std::string money(double v) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << v;
        return oss.str();
    }
}

struct Holding {
    int quantity = 0;
    double avgBuyPrice = 0.0; 
};

class Portfolio {
private:
    double cash;
    std::unordered_map<std::string, Holding> holdings; 
    TransactionStack history;
    double totalRealizedPnL = 0.0;

public:
    explicit Portfolio(double startingCash) : cash(startingCash) {}

    double getCash() const { return cash; }
    double getRealizedPnL() const { return totalRealizedPnL; }
    const TransactionStack& getHistory() const { return history; }

    int quantityHeld(const std::string& symbol) const {
        auto it = holdings.find(symbol);
        return it == holdings.end() ? 0 : it->second.quantity;
    }

    double avgBuyPrice(const std::string& symbol) const {
        auto it = holdings.find(symbol);
        return it == holdings.end() ? 0.0 : it->second.avgBuyPrice;
    }

    const std::unordered_map<std::string, Holding>& allHoldings() const { return holdings; }

    bool buy(const std::string& symbol, int qty, double price, std::string& message) {
        double cost = qty * price;
        if (qty <= 0) { message = "Quantity must be positive."; return false; }
        if (cost > cash) { message = "Insufficient funds."; return false; }

        Holding& h = holdings[symbol];
        double totalCostBasis = h.avgBuyPrice * h.quantity + cost;
        h.quantity += qty;
        h.avgBuyPrice = totalCostBasis / h.quantity;
        cash -= cost;

        Transaction t{TxType::BUY, symbol, qty, price, 0.0, std::time(nullptr)};
        history.push(t);
        message = "Bought " + std::to_string(qty) + " share(s) of " + symbol +
                   " for " + currency::SYMBOL + detail::money(cost);
        return true;
    }


    bool sell(const std::string& symbol, int qty, double price, std::string& message) {
        auto it = holdings.find(symbol);
        if (qty <= 0) { message = "Quantity must be positive."; return false; }
        if (it == holdings.end() || it->second.quantity < qty) {
            message = "You don't own enough shares to sell.";
            return false;
        }

        Holding& h = it->second;
        double proceeds = qty * price;
        double costBasis = h.avgBuyPrice * qty;
        double realized = proceeds - costBasis;

        h.quantity -= qty;
        cash += proceeds;
        totalRealizedPnL += realized;
        if (h.quantity == 0) h.avgBuyPrice = 0.0;

        Transaction t{TxType::SELL, symbol, qty, price, realized, std::time(nullptr)};
        history.push(t);

        message = std::string(realized >= 0 ? "Profit of " : "Loss of ") +
                  currency::SYMBOL + detail::money(std::fabs(realized)) +
                  " realized on this sale.";
        return true;
    }


    bool undoLast(std::string& message) {
        if (history.isEmpty()) { message = "No transactions to undo."; return false; }
        Transaction t = history.pop();

        if (t.type == TxType::BUY) {
            Holding& h = holdings[t.symbol];

            h.quantity -= t.quantity;
            cash += t.total();
            if (h.quantity <= 0) { h.quantity = 0; h.avgBuyPrice = 0.0; }
            message = "Undid BUY of " + std::to_string(t.quantity) + " " + t.symbol;
        } else {
            Holding& h = holdings[t.symbol];
            h.quantity += t.quantity;
            cash -= t.total();
            totalRealizedPnL -= t.realizedPnL;
            message = "Undid SELL of " + std::to_string(t.quantity) + " " + t.symbol;
        }
        return true;
    }

    bool saveToFile(const std::string& path) const {
        std::ofstream out(path, std::ios::trunc);
        if (!out.is_open()) return false;
        out << std::setprecision(12);
        out << "R," << totalRealizedPnL << "\n";
        for (const auto& [symbol, h] : holdings) {
            if (h.quantity > 0)
                out << "H," << symbol << "," << h.quantity << "," << h.avgBuyPrice << "\n";
        }
        for (const Transaction& t : history.oldestFirst()) {
            out << "T," << (t.type == TxType::BUY ? 0 : 1) << "," << t.symbol << ","
                << t.quantity << "," << t.priceEach << "," << t.realizedPnL << ","
                << static_cast<long long>(t.timestamp) << "\n";
        }
        return true;
    }

    bool loadFromFile(const std::string& path) {
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
                    totalRealizedPnL = std::stod(*std::next(f.begin(), 1));
                } else if (f.front() == "H" && f.size() >= 4) {
                    auto it = std::next(f.begin());
                    Holding h;
                    std::string symbol = *it++;
                    h.quantity    = std::stoi(*it++);
                    h.avgBuyPrice = std::stod(*it);
                    holdings[symbol] = h;
                } else if (f.front() == "T" && f.size() >= 7) {
                    auto it = std::next(f.begin());
                    Transaction t;
                    t.type        = (std::stoi(*it++) == 0) ? TxType::BUY : TxType::SELL;
                    t.symbol      = *it++;
                    t.quantity    = std::stoi(*it++);
                    t.priceEach   = std::stod(*it++);
                    t.realizedPnL = std::stod(*it++);
                    t.timestamp   = static_cast<std::time_t>(std::stoll(*it));
                    history.push(t);
                }
            } catch (...) {

            }
        }
        return true;
    }

    double unrealizedPnL(const std::string& symbol, double currentPrice) const {
        auto it = holdings.find(symbol);
        if (it == holdings.end() || it->second.quantity == 0) return 0.0;
        return (currentPrice - it->second.avgBuyPrice) * it->second.quantity;
    }
};

#endif 
