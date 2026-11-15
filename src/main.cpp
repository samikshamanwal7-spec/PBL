#include <iostream>
#include <iomanip>
#include <limits>
#include <string>
#include <sys/stat.h>

#include "../include/Console.h"
#include "../include/User.h"
#include "../include/Market.h"
#include "../include/Portfolio.h"
#include "../include/OrderQueue.h"

// ============================================================
// main.cpp
// Stock Market Trading Simulator (offline, C++, console/TUI)
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
    mkdir("data", 0755); // no-op if it already exists
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
        ui::banner("Stock Market Trading Simulator");
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
                ui::success("Account created! Starting balance: $10000.00");
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

static void viewPortfolio(Portfolio& portfolio, Market& market) {
    ui::title("Your Portfolio");
    std::cout << "Cash Balance: " << ui::GREEN << "$" << std::fixed << std::setprecision(2)
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
    std::cout << "Holdings value: $" << std::fixed << std::setprecision(2) << holdingsValue << "\n";
    std::cout << "Net worth (cash + holdings): " << ui::BOLD << "$"
               << (portfolio.getCash() + holdingsValue) << ui::RESET << "\n";
    std::cout << "Total unrealized P&L: " << ui::colorSigned(totalUnrealized) << "\n";
}

static void buyFlow(Market& market, Portfolio& portfolio) {
    ui::title("Buy Shares");
    std::string sym = readLine("Symbol to buy (e.g. TCS): ");
    Stock* s = market.find(sym);
    if (!s) { ui::error("Unknown symbol."); return; }
    std::cout << "Current price of " << s->getSymbol() << ": $" << std::fixed
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
    std::cout << "Current price: $" << std::fixed << std::setprecision(2) << s->getPrice() << "\n";
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
                   << " @ $" << std::fixed << std::setprecision(2) << t.priceEach;
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

// ---------------- Main dashboard loop ----------------
static void dashboard(User& user, AuthManager& auth, Market& market) {
    Portfolio portfolio(user.getCashBalance());

    while (true) {
        ui::clearScreen();
        ui::title("Dashboard -- " + user.getUsername());
        std::cout << "Cash: $" << std::fixed << std::setprecision(2) << portfolio.getCash() << "\n\n";
        ui::menuBox("40% Evaluator Milestone", {
            "View Market",
            "Buy Shares",
            "Sell Shares",
            "View Portfolio",
            "Transaction History",
            "Undo Last Transaction",
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
            case 7: {
                user.setCashBalance(portfolio.getCash());
                auth.saveBalance(user);
                ui::success("Balance saved. Goodbye, " + user.getUsername() + "!");
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
