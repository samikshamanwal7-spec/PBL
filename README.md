# Stock Market Trading Simulator — 40% Evaluator Milestone

This version intentionally exposes the first demonstrable milestone rather than every planned feature.

## What you can demonstrate
1. Sign up and sign in using a local file (`data/users.db`).
2. View the stock catalogue, sorted by ticker using a Binary Search Tree (BST).
3. Buy shares with virtual cash; the program checks positive quantity and sufficient funds.
4. Sell shares only when enough shares are held.
5. View holdings, cash, realized profit/loss, and unrealized profit/loss.
6. View recent transactions and undo the most recent buy/sell using a stack.
7. Save the remaining cash balance when exiting.

## Build
From this `stock_sim` directory:

```bash
g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp src/OrderQueue.cpp -o stock_sim
```

Then run `./stock_sim` on Linux/macOS/WSL or `stock_sim.exe` on Windows.

## Important limitation to disclose
This is a milestone/demo, not a production trading system. Only the user's cash balance is persisted at exit. Holdings and transaction history are currently kept in memory and reset when the program restarts. Prices are simulated/static during this milestone because the market-tick option is hidden. The underlying code still contains later-stage graph, price-history, moving-average, and simulated-market modules, but they are not exposed in this milestone menu.

The login password uses `std::hash`, which is not a secure password-hashing algorithm. Use only sample credentials for evaluation.
