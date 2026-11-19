#include <iostream>
#include <iomanip>
#include <string>
#include <filesystem>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "common/Currency.h"
#include "common/Console.h"
#include "common/InputHelpers.h"

#include "modules/user_management/User.h"
#include "modules/user_management/UserInterface.h"

#include "modules/stock_market/Market.h"
#include "modules/stock_market/MarketInterface.h"

#include "modules/trading_portfolio/Portfolio.h"
#include "modules/trading_portfolio/TradingInterface.h"

#include "modules/transactions_orders/OrderQueue.h"
#include "modules/transactions_orders/TransactionStorage.h"
#include "modules/transactions_orders/TransactionInterface.h"

// ============================================================
// main.cpp
// Virtual Stock Market Learning Platform (offline, C++, console/TUI)
//
// main.cpp only wires the four modules together. Each module has
// its own backend (data structures + logic) and interface (screens):
//
//   User Management        User.h                  UserInterface.h
//   Stock Market           Market.h, Stock.h, ...  MarketInterface.h
//   Trading & Portfolio    Portfolio.h             TradingInterface.h
//   Transactions & Orders  OrderQueue.h, ...       TransactionInterface.h
//
// Flow: Sign Up / Sign In  ->  Trading Dashboard
//   1. View Market            2. Buy Shares          3. Sell Shares
//   4. View Portfolio         5. Transaction History 6. Undo Last
//   7. Related Stocks         8. Price Trend Chart   9. Advance Market
//  10. Save & Exit
// ============================================================

static void ensureDataDir() {
    std::filesystem::create_directories("data"); // no-op if it already exists
}

// ---------------- Main dashboard loop ----------------
static void dashboard(User& user, AuthManager& auth, Market& market) {
    Portfolio portfolio(user.getCashBalance());
    const std::string portfolioFile = TransactionStorage::pathFor(user.getUsername());
    TransactionStorage::load(portfolio, portfolioFile); // restore holdings, P&L, history

    while (true) {
        ui::clearScreen();
        ui::title("Dashboard -- " + user.getUsername());
        std::cout << "Cash: " + currency::SYMBOL << std::fixed << std::setprecision(2) << portfolio.getCash() << "\n\n";
        ui::menuBox("Main Menu", {
            "View Market",
            "Buy Shares",
            "Sell Shares",
            "View Portfolio",
            "Transaction History",
            "Undo Last Transaction",
            "Related Stocks (sector graph)",
            "Price Trend Chart",
            "Advance Market (next tick)",
            "Save & Exit"
        });
        int choice = input::readInt();
        std::cout << "\n";

        switch (choice) {
            case 1: viewMarket(market); break;                 // Stock Market
            case 2: buyFlow(market, portfolio); break;         // Trading & Portfolio
            case 3: sellFlow(market, portfolio); break;        // Trading & Portfolio
            case 4: viewPortfolio(portfolio, market); break;   // Trading & Portfolio
            case 5: historyView(portfolio); break;             // Transactions & Orders
            case 6: undoFlow(portfolio); break;                // Transactions & Orders
            case 7: relatedStocksFlow(market); break;          // Stock Market
            case 8: trendFlow(market); break;                  // Stock Market
            case 9: advanceMarket(market); break;              // Stock Market
            case 10: {
                user.setCashBalance(portfolio.getCash());
                auth.saveBalance(user);                        // User Management
                TransactionStorage::save(portfolio, portfolioFile); // Transactions & Orders
                ui::success("Balance and portfolio saved. Goodbye, " + user.getUsername() + "!");
                ui::pause();
                return;
            }
            default:
                ui::error("Invalid choice.");
        }
        ui::pause();
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // so the rupee sign displays correctly
#endif
    ensureDataDir();
    AuthManager auth("data/users.db");
    Market market;

    User currentUser;
    if (!authFlow(auth, currentUser)) {
        std::cout << "Goodbye!\n";
        return 0;
    }

    dashboard(currentUser, auth, market);
    return 0;
}
