# Virtual Stock Market Learning Platform (Offline, C++)

A menu-driven desktop-style trading simulator, written entirely in
standard C++17 with **no external libraries** — so it builds and runs
fully offline on Linux, macOS, or Windows (MinGW/WSL) with nothing but
`g++`.

It uses a colored, boxed console interface (a "TUI") rather than a
graphical toolkit like Qt or SFML, specifically so you don't need to
install anything extra to compile and run it. If you *do* want a true
graphical window later, the entire simulation engine (`Market`,
`Portfolio`, `Stock`, etc.) is UI-agnostic — only `main.cpp` talks to
the console — so it can be re-wired to a Qt/SFML front end without
touching the logic.

## Features

- Sign up / sign in, persisted locally to `data/users.db` (offline, file-based)
- Holdings, realized P&L and transaction history are saved per user to `data/portfolio_<username>.db` on Save & Exit and restored on the next sign in
- All amounts are shown in Indian rupees (₹); the symbol is defined once in `include/Currency.h`
- Live-simulated market of 10 stocks across 4 sectors
- Buy / sell shares with real-time cash and profit/loss tracking
- Realized P&L (from closed trades) and unrealized P&L (from open positions)
- Undo the last transaction
- Transaction history
- Sector-correlated price movement ("related stocks")
- Price trend mini-chart with moving average

## How every DSA concept is actually used (not just bolted on)

| Concept | Class | What it's actually doing |
|---|---|---|
| **Doubly Linked List** | `PriceHistory` (`include/PriceHistory.h`) | Each `Stock` stores its tick-by-tick price history as a linked list. Appending a new tick is O(1); walking `prev` pointers computes moving averages and trend charts without needing a resizable array. |
| **Stack (LIFO)** | `TransactionStack` (`include/TransactionStack.h`) | Every completed buy/sell is pushed. "Undo Last Transaction" pops the most recent one and reverses it — a stack is the natural fit because only the *most recent* action can be sensibly undone. |
| **Queue (FIFO)** | `OrderQueue` (`include/OrderQueue.h`) | When you place a buy/sell it's enqueued as a pending `Order`, then the market engine drains the queue first-in-first-out against the current price — a simplified model of how an exchange's order book executes orders in arrival order. |
| **Binary Search Tree** | `StockBST` (`include/StockBST.h`) | All stocks are indexed by ticker symbol in a BST for O(log n) average lookup. An in-order traversal is used to print the market listing alphabetically for free. |
| **Graph (adjacency list + BFS)** | `StockGraph` (`include/StockGraph.h`) | Stocks in the same sector are connected by edges. When a stock's price moves, a BFS out to its neighbors nudges correlated stocks in the same direction (decayed per hop) — and the same BFS powers the "Related Stocks" menu option. |

## Sequence containers

All general-purpose sequence storage (stock list, seed data, BFS results, neighbour lists, CSV fields, menu options, transaction storage under the stack, etc.) uses `std::list` (a doubly linked list) instead of `std::vector`. Code that previously used indexed access (`v[i]`) now uses iterators, range-for, `front()`/`back()`, or `std::next`.

## How OOP is used

- **Encapsulation**: `User`, `Stock`, `Portfolio` keep their state private behind accessor methods.
- **Composition**: `Stock` *has-a* `PriceHistory`; `Portfolio` *has-a* `TransactionStack`; `Market` *has-a* `StockBST` + `StockGraph` + `OrderQueue`.
- **Inheritance & Polymorphism**: `Order` is an abstract base class; `BuyOrder` and `SellOrder` override `execute()`. The market engine calls `order->execute(...)` without knowing which subclass it holds — classic runtime polymorphism.
- **Abstraction**: `main.cpp` (the UI layer) only ever calls high-level methods like `portfolio.buy(...)` or `market.tick()` — it never touches a linked-list node or a BST pointer directly.

## Build & Run

Requires only a C++17 compiler (g++ 9+, clang 10+, or MSVC 2019+).

```bash
# Using the provided Makefile (Linux/macOS/WSL):
make
./stock_sim

# Or compile directly:
g++ -std=c++17 -O2 -Iinclude src/main.cpp src/OrderQueue.cpp -o stock_sim
./stock_sim
```

On plain Windows without WSL, install MinGW-w64 and run the same `g++`
command from a terminal (or open the folder in Visual Studio / VS Code
and build `src/main.cpp` + `src/OrderQueue.cpp` as a console app).

## Project Layout

```
stock_sim/
├── include/
│   ├── Currency.h        Currency symbol (rupee) used everywhere
│   ├── Console.h         UI helpers: colors, boxes, tables (the "desktop" look)
│   ├── User.h            User model + file-based sign up / sign in
│   ├── PriceHistory.h    Doubly linked list of price ticks
│   ├── Stock.h           One listed stock (has-a PriceHistory)
│   ├── TransactionStack.h  Stack of buy/sell transactions (undo support)
│   ├── OrderQueue.h      Order base class + BuyOrder/SellOrder + FIFO queue
│   ├── StockBST.h        Binary search tree index of stocks by symbol
│   ├── StockGraph.h      Sector correlation graph + BFS
│   ├── Portfolio.h       Cash, holdings, P&L, buy/sell/undo logic, save/load to file
│   └── Market.h          Wires all the above together, simulates price ticks
├── src/
│   ├── main.cpp           Entry point: auth screen + dashboard menu (the only file that "does UI")
│   └── OrderQueue.cpp    Out-of-line Order::execute() implementations
├── Makefile
└── README.md
```

## A note on the login system

Passwords are hashed with `std::hash` before being written to
`data/users.db` purely so the file doesn't contain plaintext. **This
is for a learning project only — `std::hash` is not a cryptographic
hash and this should never be used for a real authentication
system.** For production-grade auth you'd want a proper salted hash
(bcrypt/argon2) and, ideally, not to write your own auth layer at all.

## Ideas to extend it

- Swap `main.cpp` for a Qt or SFML front end (the engine underneath doesn't change)
- Add a `Heap`/priority queue for "top gainers/losers" leaderboard
- Add limit orders (only execute when price crosses a threshold) to make the `OrderQueue` do more real work
