#ifndef STOCK_H
#define STOCK_H

#include <string>
#include "PriceHistory.h"

// ============================================================
// Stock.h -- OOP: ENCAPSULATION
// Holds identity + current state of one listed stock. The
// price history (linked list) is owned here, showing
// composition (a Stock "has-a" PriceHistory).
// ============================================================
class Stock {
private:
    std::string symbol;
    std::string name;
    std::string sector;      // used by StockGraph for correlation edges
    PriceHistory history;

public:
    Stock(std::string sym, std::string nm, std::string sec, double startPrice)
        : symbol(std::move(sym)), name(std::move(nm)), sector(std::move(sec)) {
        history.append(startPrice);
    }

    const std::string& getSymbol() const { return symbol; }
    const std::string& getName() const { return name; }
    const std::string& getSector() const { return sector; }

    double getPrice() const { return history.latest(); }
    double getPrevPrice() const { return history.previous(); }
    const PriceHistory& getHistory() const { return history; }

    double getDayChangePercent() const {
        double prev = getPrevPrice();
        if (prev == 0.0) return 0.0;
        return ((getPrice() - prev) / prev) * 100.0;
    }

    // Applies a price move (percentage delta, e.g. +2.5 means +2.5%).
    void applyPercentMove(double percent) {
        double newPrice = getPrice() * (1.0 + percent / 100.0);
        if (newPrice < 0.05) newPrice = 0.05; // floor so prices never hit 0/negative
        history.append(newPrice);
    }
};

#endif // STOCK_H
