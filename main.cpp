#include <iostream>
#include <iomanip>
#include <limits>
#include <string>
#include <filesystem>
#include <cctype>
#include <cstdio>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "../include/Currency.h"
#include "../include/Console.h"
#include "../include/User.h"
#include "../include/Market.h"
#include "../include/Portfolio.h"
#include "../include/OrderQueue.h"

// ============================================================
// main.cpp
// Virtual Stock Market Learning Platform (offline, C++, console/TUI)
//
// Flow: Sign Up / Sign In  ->  Trading Dashboard
//   1. View Market            (BST in-order listing)
//   2. Buy Shares              (Queue -> Order -> Portfolio)
//   3. Sell Shares             (Queue -> Order -> Portfolio)
//   4. View Portfolio          (holdings + unrealized P&L)
//   5. Transaction History     (Stack, most-recent-first)
//   6. Undo Last Transaction   (Stack pop)
//   7. Related Stocks          (Graph BFS by sector)
//   8. Price Trend / Chart     (Linked List traversal)
//   9. Advance Market (next tick + process order queue)
//   0. Save & Exit
// ============================================================

static void ensureDataDir() {
    std::filesystem::create_directories("data"); // no-op if it already exists
}

// data/portfolio_<username>.db -- non-alphanumeric characters are
// hex-encoded so any username maps to a safe, unique file name.
static std::string portfolioPath(const std::string& username) {
    std::string safe;
    for (unsigned char c : username) {
        if (std::isalnum(c)) safe += static_cast<char>(c);
        else { char buf[4]; std::snprintf(buf, sizeof buf, "_%02X", c); safe += buf; }
    }
    return "data/portfolio_" + safe + ".db";
}

static int readInt() {
    int v;
    while (!(std::cin >> v)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        ui::error("Please enter a valid number.");
        std::cout << " > ";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return v;
}

static std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// ---------------- Authentication screen ----------------
static bool authFlow(AuthManager& auth, User& currentUser) {
    while (true) {
        ui::banner("Virtual Stock Market Learning Platform");
        ui::menuBox("Welcome", {"Sign In", "Sign Up", "Exit"});
        int choice = readInt();

        if (choice == 1) {
            std::string uname = readLine("Username: ");
            std::string pass  = readLine("Password: ");
            if (auth.signIn(uname, pass, currentUser)) {
                ui::success("Welcome back, " + uname + "!");
                ui::pause();
                return true;
            } else {
                ui::error("Invalid username or password.");
                ui::pause();
            }
        } else if (choice == 2) {
            std::string uname = readLine("Choose a username: ");
            std::string pass  = readLine("Choose a password: ");
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

// ---------------- Dashboard views ----------------
static void viewMarket(Market& market) {
    ui::title("Market -- All Stocks )");
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

static void viewPortfolio(Portfolio& portfolio, Market& market) {
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

static void buyFlow(Market& market, Portfolio& portfolio) {
    ui::title("Buy Shares");
    std::string sym = readLine("Symbol to buy (e.g. TCS): ");
    Stock* s = market.find(sym);
    if (!s) { ui::error("Unknown symbol."); return; }
    std::cout << "Current price of " << s->getSymbol() << ": " + currency::SYMBOL << std::fixed
               << std::setprecision(2) << s->getPrice() << "\n";
    std::cout << "Quantity: ";
    int qty = readInt();

    market.placeOrder(std::make_shared<BuyOrder>(s->getSymbol(), qty));
    auto results = market.processOrders(portfolio);
    for (auto& r : results) std::cout << r << "\n";
}

static void sellFlow(Market& market, Portfolio& portfolio) {
    ui::title("Sell Shares");
    std::string sym = readLine("Symbol to sell (e.g. TCS): ");
    Stock* s = market.find(sym);
    if (!s) { ui::error("Unknown symbol."); return; }
    int held = portfolio.quantityHeld(s->getSymbol());
    std::cout << "You currently hold " << held << " share(s) of " << s->getSymbol() << "\n";
    std::cout << "Current price: " + currency::SYMBOL << std::fixed << std::setprecision(2) << s->getPrice() << "\n";
    std::cout << "Quantity to sell: ";
    int qty = readInt();

    market.placeOrder(std::make_shared<SellOrder>(s->getSymbol(), qty));
    auto results = market.processOrders(portfolio);
    for (auto& r : results) std::cout << r << "\n";
}

static void historyView(Portfolio& portfolio) {
    ui::title("Transaction History (Stack -- most recent first)");
    auto txs = portfolio.getHistory().recentFirst();
    if (txs.empty()) { ui::info("No transactions yet."); return; }
    for (auto& t : txs) {
        std::string typeStr = (t.type == TxType::BUY) ? (ui::YELLOW + "BUY " + ui::RESET)
                                                        : (ui::CYAN + "SELL" + ui::RESET);
        std::cout << typeStr << " " << std::left << std::setw(6) << t.symbol
                   << " qty=" << std::setw(5) << t.quantity
                   << " @ " + currency::SYMBOL << std::fixed << std::setprecision(2) << t.priceEach;
        if (t.type == TxType::SELL) std::cout << "  P&L: " << ui::colorSigned(t.realizedPnL);
        std::cout << "\n";
    }
}

static void undoFlow(Portfolio& portfolio) {
    ui::title("Undo Last Transaction");
    std::string msg;
    if (portfolio.undoLast(msg)) ui::success(msg);
    else ui::error(msg);
}

static void relatedStocksFlow(Market& market) {
    ui::title("Related Stocks (Graph BFS by sector)");
    std::string sym = readLine("Symbol: ");
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

static void trendFlow(Market& market) {
    ui::title("Price Trend (Linked List traversal)");
    std::string sym = readLine("Symbol: ");
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

// ---------------- Main dashboard loop ----------------
static void dashboard(User& user, AuthManager& auth, Market& market) {
    Portfolio portfolio(user.getCashBalance());
    portfolio.loadFromFile(portfolioPath(user.getUsername())); // restore holdings, P&L, history

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
        int choice = readInt();
        std::cout << "\n";

        switch (choice) {
            case 1: viewMarket(market); break;
            case 2: buyFlow(market, portfolio); break;
            case 3: sellFlow(market, portfolio); break;
            case 4: viewPortfolio(portfolio, market); break;
            case 5: historyView(portfolio); break;
            case 6: undoFlow(portfolio); break;
            case 7: relatedStocksFlow(market); break;
            case 8: trendFlow(market); break;
            case 9:
                market.tick();
                ui::success("Market advanced one tick. Prices updated.");
                break;
            case 10: {
                user.setCashBalance(portfolio.getCash());
                auth.saveBalance(user);
                portfolio.saveToFile(portfolioPath(user.getUsername()));
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
