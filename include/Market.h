#ifndef MARKET_H
#define MARKET_H

#include <vector>
#include <memory>
#include <random>
#include <string>
#include "Stock.h"
#include "StockBST.h"
#include "StockGraph.h"
#include "OrderQueue.h"
#include "Portfolio.h"

// ============================================================
// Market.h -- ties every DSA structure together:
//   - owns the Stock objects (source of truth for price)
//   - indexes them in a StockBST (fast lookup + sorted listing)
//   - links them in a StockGraph (sector correlation)
//   - drains an OrderQueue each tick (FIFO order execution)
// ============================================================

class Market {
private:
    std::vector<std::unique_ptr<Stock>> stocks;
    StockBST index;
    StockGraph graph;
    OrderQueue pendingOrders;
    std::mt19937 rng{std::random_device{}()};

    void seed() {
        struct Seed { std::string sym, name, sector; double price; };
        std::vector<Seed> seeds = {
            {"TCS",  "Tata Consultancy Services", "IT",      3850.0},
            {"INFY", "Infosys Ltd",               "IT",      1620.0},
            {"WIPRO","Wipro Ltd",                 "IT",       485.0},
            {"HDFC", "HDFC Bank",                 "BANKING", 1650.0},
            {"ICICI","ICICI Bank",                "BANKING", 1180.0},
            {"SBIN", "State Bank of India",       "BANKING",  815.0},
            {"RELI", "Reliance Industries",       "ENERGY",  2950.0},
            {"ONGC", "Oil & Natural Gas Corp",    "ENERGY",   265.0},
            {"TATM", "Tata Motors",               "AUTO",     975.0},
            {"MRTI", "Maruti Suzuki",             "AUTO",   12100.0},
        };
        for (auto& s : seeds) {
            stocks.push_back(std::make_unique<Stock>(s.sym, s.name, s.sector, s.price));
            index.insert(stocks.back().get());
            graph.addNode(s.sym);
        }
        // Connect same-sector stocks so moves correlate (graph edges).
        for (size_t i = 0; i < seeds.size(); i++)
            for (size_t j = i + 1; j < seeds.size(); j++)
                if (seeds[i].sector == seeds[j].sector)
                    graph.addEdge(seeds[i].sym, seeds[j].sym);
    }

public:
    Market() { seed(); }

    Stock* find(const std::string& symbol) const { return index.find(symbol); }
    std::vector<Stock*> allSorted() const { return index.allSortedBySymbol(); }
    const StockGraph& getGraph() const { return graph; }

    void placeOrder(std::shared_ptr<Order> order) { pendingOrders.enqueue(order); }

    // Drains the FIFO order queue against current market prices.
    // Returns human-readable result lines.
    std::vector<std::string> processOrders(Portfolio& portfolio) {
        std::vector<std::string> results;
        while (!pendingOrders.isEmpty()) {
            auto order = pendingOrders.dequeue();
            Stock* s = index.find(order->getSymbol());
            if (!s) { results.push_back("Rejected: unknown symbol " + order->getSymbol()); continue; }
            std::string msg;
            bool ok = order->execute(portfolio, s->getPrice(), msg);
            results.push_back((ok ? "[" + order->label() + " OK] " : "[" + order->label() + " REJECTED] ") + msg);
        }
        return results;
    }

    // Advances the whole market by one simulated tick: every
    // stock gets a small random walk, then correlated sector
    // neighbours (via BFS on the graph) get a decayed nudge in
    // the same direction.
    void tick() {
        std::normal_distribution<double> baseMove(0.0, 1.2); // % move, mean 0, sd 1.2
        std::vector<double> primaryMoves(stocks.size());

        for (size_t i = 0; i < stocks.size(); i++) {
            primaryMoves[i] = baseMove(rng);
            stocks[i]->applyPercentMove(primaryMoves[i]);
        }
        // Propagate correlated nudges via BFS (graph traversal).
        for (size_t i = 0; i < stocks.size(); i++) {
            auto reached = graph.bfsWithinHops(stocks[i]->getSymbol(), 1);
            for (auto& [sym, dist] : reached) {
                Stock* neighbor = index.find(sym);
                if (neighbor) neighbor->applyPercentMove(primaryMoves[i] * 0.25 / dist);
            }
        }
    }
};

#endif // MARKET_H
