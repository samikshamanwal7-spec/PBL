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
- All amounts are shown in Indian rupees (₹); the symbol is defined once in `common/Currency.h`
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
| **Doubly Linked List** | `PriceHistory` (`modules/stock_market/PriceHistory.h`) | Each `Stock` stores its tick-by-tick price history as a linked list. Appending a new tick is O(1); walking `prev` pointers computes moving averages and trend charts without needing a resizable array. |
| **Stack (LIFO)** | `TransactionStack` (`modules/transactions_orders/TransactionStack.h`) | Every completed buy/sell is pushed. "Undo Last Transaction" pops the most recent one and reverses it — a stack is the natural fit because only the *most recent* action can be sensibly undone. |
| **Queue (FIFO)** | `OrderQueue` (`modules/transactions_orders/OrderQueue.h`) | When you place a buy/sell it's enqueued as a pending `Order`, then the market engine drains the queue first-in-first-out against the current price — a simplified model of how an exchange's order book executes orders in arrival order. |
| **Binary Search Tree** | `StockBST` (`modules/stock_market/StockBST.h`) | All stocks are indexed by ticker symbol in a BST for O(log n) average lookup. An in-order traversal is used to print the market listing alphabetically for free. |
| **Graph (adjacency list + BFS)** | `StockGraph` (`modules/stock_market/StockGraph.h`) | Stocks in the same sector are connected by edges. When a stock's price moves, a BFS out to its neighbors nudges correlated stocks in the same direction (decayed per hop) — and the same BFS powers the "Related Stocks" menu option. |

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
g++ -std=c++17 -O2 -I. src/main.cpp modules/transactions_orders/OrderQueue.cpp -o stock_sim
./stock_sim
```

On plain Windows without WSL, install MinGW-w64 and run the same `g++`
command from a terminal (or open the folder in Visual Studio / VS Code
and build `src/main.cpp` + `modules/transactions_orders/OrderQueue.cpp` as a console app).

## Project Layout

The code is divided into four modules. Each module has a **backend** (data
structures and logic) and an **interface** (the console screens for that
module). `main.cpp` only wires the modules together.

```
stock_sim/
├── src/
│   └── main.cpp                       Entry point: dashboard menu, calls each module's screens
├── common/                            Shared by all modules
│   ├── Console.h                      UI helpers: colors, boxes, tables
│   ├── Currency.h                     Currency symbol (rupee) used everywhere
│   └── InputHelpers.h                 Validated number / text input
├── modules/
│   ├── user_management/               MODULE 1 - Login, registration, authentication
│   │   ├── User.h                       Backend: User + AuthManager, users.db file handling
│   │   └── UserInterface.h              Interface: Sign In / Sign Up screen
│   ├── stock_market/                  MODULE 2 - Stock data, price updates, search, sort
│   │   ├── Stock.h                      One listed stock (has-a PriceHistory)
│   │   ├── PriceHistory.h               Doubly linked list of price ticks
│   │   ├── StockBST.h                   Binary search tree (search + sorted listing)
│   │   ├── StockGraph.h                 Sector correlation graph + BFS
│   │   ├── Market.h                     Market engine: seeding, price ticks, order processing
│   │   └── MarketInterface.h            Interface: market table, related stocks, trend chart, advance market
│   ├── trading_portfolio/             MODULE 3 - Buy/sell, balance, holdings, profit & loss
│   │   ├── Portfolio.h                  Backend: cash, holdings (hash map), realized/unrealized P&L
│   │   └── TradingInterface.h           Interface: buy, sell, view portfolio
│   └── transactions_orders/           MODULE 4 - History, order processing, stack + queue, file handling
│       ├── OrderQueue.h / .cpp          Order base class + BuyOrder/SellOrder + FIFO queue
│       ├── TransactionStack.h           Stack of buy/sell transactions (undo support)
│       ├── TransactionStorage.h         File handling: saves / restores portfolio + history
│       └── TransactionInterface.h       Interface: transaction history, undo
├── Makefile
└── README.md
```

## Web front end (local page, works offline)

A browser-based front end is included alongside the console version. It does
**not** change any module logic: `web/WebApi.h` simply calls the same backend
classes (`AuthManager`, `Market`, `Portfolio`, `TransactionStorage`) that the
console screens call, and answers the browser with JSON.

```
web/
├── index.html      The page (HTML + CSS + JavaScript in one file, no internet resources)
├── HttpServer.h    Tiny HTTP server on the OS socket API, listens on 127.0.0.1 only
└── WebApi.h        JSON endpoints that call the existing modules
src/web_main.cpp    Entry point of the web version
```

Build and run (from the `stock_sim` folder):

```
# Windows (MSYS2 / MinGW)
g++ -std=c++17 -I. -pthread src/web_main.cpp modules/transactions_orders/OrderQueue.cpp -o stocksim_web.exe -lws2_32
.\stocksim_web.exe

# Linux / macOS
make run-web
```

Then open `http://127.0.0.1:8080` in a browser (on Windows it opens automatically).
It needs no internet; the server only accepts connections from this computer.
The web version also saves cash and portfolio after every trade, not just on sign out.
The console version (`src/main.cpp`) is unchanged and still works.

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
