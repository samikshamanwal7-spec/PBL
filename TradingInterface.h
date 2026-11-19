#ifndef TRADING_INTERFACE_H
#define TRADING_INTERFACE_H

#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include "common/Currency.h"
#include "common/Console.h"
#include "common/InputHelpers.h"
#include "modules/stock_market/Market.h"
#include "modules/trading_portfolio/Portfolio.h"
#include "modules/transactions_orders/OrderQueue.h"

// ============================================================
// TradingInterface.h -- Trading & Portfolio module (interface)
// Screens: buy shares, sell shares, view portfolio (P&L).
// ============================================================

inline void viewPortfolio(Portfolio& portfolio, Market& market) {
    ui::title("Your Portfolio");
    std::cout << "Cash Balance: " << ui::GREEN << currency::SYMBOL << std::fixed << std::setprecision(2)
              << portfolio.getCash() << ui::RESET << "\n";
    std::cout << "Realized P&L (all-time): " << ui::colorSigned(portfolio.getRealizedPnL()) << "\n\n";

    double holdingsValue = 0.0, totalUnrealized = 0.0;
    std::cout << std::left << std::setw(8) << "SYMBOL" << std::right << std::setw(8) << "QTY"
               << std::setw(12) << "AVG COST" << std::setw(12) << "PRICE" << std::setw(16) << "UNREALIZED P&L" << "\n";
    ui::drawLine(56);
    for (auto& [symbol, holding] : portfolio.allHoldings()) {
        if (holding.quantity <= 0) continue;
        Stock* s = market.find(symbol);
        double price = s ? s->getPrice() : holding.avgBuyPrice;
        double unreal = portfolio.unrealizedPnL(symbol, price);
        holdingsValue += price * holding.quantity;
        totalUnrealized += unreal;
        std::cout << std::left << std::setw(8) << symbol << std::right << std::setw(8) << holding.quantity
                   << std::setw(12) << std::fixed << std::setprecision(2) << holding.avgBuyPrice
                   << std::setw(12) << price
                   << std::setw(10) << "  " << ui::colorSigned(unreal) << "\n";
    }
    ui::drawLine(56);
    std::cout << "Holdings value: " + currency::SYMBOL << std::fixed << std::setprecision(2) << holdingsValue << "\n";
    std::cout << "Net worth (cash + holdings): " << ui::BOLD << currency::SYMBOL
               << (portfolio.getCash() + holdingsValue) << ui::RESET << "\n";
    std::cout << "Total unrealized P&L: " << ui::colorSigned(totalUnrealized) << "\n";
}

inline void buyFlow(Market& market, Portfolio& portfolio) {
    ui::title("Buy Shares");
    std::string sym = input::readLine("Symbol to buy (e.g. TCS): ");
    Stock* s = market.find(sym);
    if (!s) { ui::error("Unknown symbol."); return; }
    std::cout << "Current price of " << s->getSymbol() << ": " + currency::SYMBOL << std::fixed
               << std::setprecision(2) << s->getPrice() << "\n";
    std::cout << "Quantity: ";
    int qty = input::readInt();

    market.placeOrder(std::make_shared<BuyOrder>(s->getSymbol(), qty));
    auto results = market.processOrders(portfolio);
    for (auto& r : results) std::cout << r << "\n";
}

inline void sellFlow(Market& market, Portfolio& portfolio) {
    ui::title("Sell Shares");
    std::string sym = input::readLine("Symbol to sell (e.g. TCS): ");
    Stock* s = market.find(sym);
    if (!s) { ui::error("Unknown symbol."); return; }
    int held = portfolio.quantityHeld(s->getSymbol());
    std::cout << "You currently hold " << held << " share(s) of " << s->getSymbol() << "\n";
    std::cout << "Current price: " + currency::SYMBOL << std::fixed << std::setprecision(2) << s->getPrice() << "\n";
    std::cout << "Quantity to sell: ";
    int qty = input::readInt();

    market.placeOrder(std::make_shared<SellOrder>(s->getSymbol(), qty));
    auto results = market.processOrders(portfolio);
    for (auto& r : results) std::cout << r << "\n";
}

#endif // TRADING_INTERFACE_H
