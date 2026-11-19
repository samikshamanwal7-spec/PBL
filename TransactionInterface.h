#ifndef TRANSACTION_INTERFACE_H
#define TRANSACTION_INTERFACE_H

#include <iostream>
#include <iomanip>
#include <string>
#include "common/Currency.h"
#include "common/Console.h"
#include "modules/trading_portfolio/Portfolio.h"

// ============================================================
// TransactionInterface.h -- Transactions & Orders module (interface)
// Screens: transaction history (stack, newest first), undo last.
// ============================================================

inline void historyView(Portfolio& portfolio) {
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

inline void undoFlow(Portfolio& portfolio) {
    ui::title("Undo Last Transaction");
    std::string msg;
    if (portfolio.undoLast(msg)) ui::success(msg);
    else ui::error(msg);
}

#endif // TRANSACTION_INTERFACE_H
