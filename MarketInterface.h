#ifndef MARKET_INTERFACE_H
#define MARKET_INTERFACE_H

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include "common/Currency.h"
#include "common/Console.h"
#include "common/InputHelpers.h"
#include "modules/stock_market/Market.h"

// ============================================================
// MarketInterface.h -- Stock Market module (interface)
// Screens: market table (BST), related stocks (graph BFS),
// price trend chart (linked list), advance market tick.
// ============================================================

inline void viewMarket(Market& market) {
    ui::title("Market -- All Stocks (sorted by symbol, BST in-order)");
    std::cout << std::left << std::setw(8) << "SYMBOL" << std::setw(28) << "NAME"
              << std::setw(10) << "SECTOR" << std::right << std::setw(12) << "PRICE"
              << std::setw(14) << "DAY CHANGE" << "\n";
    ui::drawLine(72);
    for (Stock* s : market.allSorted()) {
        std::cout << std::left << std::setw(8) << s->getSymbol()
                   << std::setw(28) << s->getName()
                   << std::setw(10) << s->getSector()
                   << std::right << std::setw(12) << std::fixed << std::setprecision(2) << s->getPrice()
                   << std::setw(6) << "  " << ui::colorSigned(s->getDayChangePercent()) << "%"
                   << "\n";
    }
}

inline void relatedStocksFlow(Market& market) {
    ui::title("Related Stocks (Graph BFS by sector)");
    std::string sym = input::readLine("Symbol: ");
    Stock* s = market.find(sym);
    if (!s) { ui::error("Unknown symbol."); return; }
    auto reached = market.getGraph().bfsWithinHops(s->getSymbol(), 2);
    if (reached.empty()) { ui::info("No correlated stocks found."); return; }
    std::cout << "Stocks correlated with " << s->getSymbol() << " (" << s->getSector() << "):\n";
    for (auto& [sym2, hops] : reached) {
        Stock* other = market.find(sym2);
        if (!other) continue;
        std::cout << "  " << ui::CYAN << sym2 << ui::RESET << " (" << other->getName()
                   << ") -- " << hops << " hop(s) away\n";
    }
}

inline void trendFlow(Market& market) {
    ui::title("Price Trend (Linked List traversal)");
    std::string sym = input::readLine("Symbol: ");
    Stock* s = market.find(sym);
    if (!s) { ui::error("Unknown symbol."); return; }

    auto prices = s->getHistory().lastN(15);
    double avg = s->getHistory().movingAverage(10);
    double sinceStart = s->getHistory().percentChangeSinceStart();

    std::cout << s->getSymbol() << " -- last " << prices.size() << " tick(s):\n";
    double maxP = 0, minP = 1e18;
    for (double p : prices) { maxP = std::max(maxP, p); minP = std::min(minP, p); }
    double range = (maxP - minP) < 0.0001 ? 1.0 : (maxP - minP);

    for (double p : prices) {
        int barLen = (int)(((p - minP) / range) * 40) + 1;
        std::cout << std::fixed << std::setprecision(2) << std::setw(10) << p << " | "
                   << ui::GREEN << std::string(barLen, '#') << ui::RESET << "\n";
    }
    std::cout << "\n10-tick moving average: " + currency::SYMBOL << std::fixed << std::setprecision(2) << avg << "\n";
    std::cout << "Change since history start: " << ui::colorSigned(sinceStart) << "%\n";
}

inline void advanceMarket(Market& market) {
    market.tick();
    ui::success("Market advanced one tick. Prices updated.");
}

#endif // MARKET_INTERFACE_H
