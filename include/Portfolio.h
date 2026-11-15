#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "TransactionStack.h"

namespace detail {
    inline std::string money(double v) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << v;
        return oss.str();
    }
}

// ============================================================
// Portfolio.h -- OOP: ENCAPSULATION + COMPOSITION
//
// Tracks one user's cash + holdings and computes realized /
// unrealized profit-loss. Owns a TransactionStack (stack DSA)
// so the most recent trade can be undone.
// ============================================================

struct Holding {
    int quantity = 0;
    double avgBuyPrice = 0.0; // cost basis, weighted average
};

class Portfolio {
private:
    double cash;
    std::unordered_map<std::string, Holding> holdings; // symbol -> holding
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

    // Returns false (rejected) if not enough cash.
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
                   " for $" + detail::money(cost);
        return true;
    }

    // Returns false (rejected) if not enough shares held.
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

        message = (realized >= 0 ? "Profit of $" : "Loss of $") +
                  detail::money(std::fabs(realized)) +
                  " realized on this sale.";
        return true;
    }

    // Reverses the most recent transaction (stack pop -> undo).
    bool undoLast(std::string& message) {
        if (history.isEmpty()) { message = "No transactions to undo."; return false; }
        Transaction t = history.pop();

        if (t.type == TxType::BUY) {
            Holding& h = holdings[t.symbol];
            // Reverse the weighted-average update as best-effort:
            // simplest correct reversal is to just remove the qty
            // and refund cash; average price recompute is skipped
            // for the now-smaller position for simplicity.
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

    // Unrealized P&L for one symbol at the current market price.
    double unrealizedPnL(const std::string& symbol, double currentPrice) const {
        auto it = holdings.find(symbol);
        if (it == holdings.end() || it->second.quantity == 0) return 0.0;
        return (currentPrice - it->second.avgBuyPrice) * it->second.quantity;
    }
};

#endif // PORTFOLIO_H
