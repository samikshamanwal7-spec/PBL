#ifndef STOCK_GRAPH_H
#define STOCK_GRAPH_H

#include <string>
#include <unordered_map>
#include <list>
#include <queue>
#include <unordered_set>

// ============================================================
// StockGraph.h -- DSA CONCEPT: GRAPH (adjacency list + BFS)
//
// Real markets aren't independent random walks -- stocks in
// the same sector tend to move together (a rally in one bank
// stock often nudges other bank stocks). We model this as an
// undirected graph: nodes = stock symbols, edges = "related"
// (same sector). When the engine moves one stock's price, it
// runs a BFS out to its neighbours (bounded by a decay factor
// per hop) and gives them a smaller correlated nudge.
//
// This also gives the user a "related stocks" lookup feature.
// ============================================================

class StockGraph {
private:
    std::unordered_map<std::string, std::list<std::string>> adj;

public:
    void addNode(const std::string& symbol) {
        adj[symbol]; // ensures key exists even with no edges yet
    }

    void addEdge(const std::string& a, const std::string& b) {
        adj[a].push_back(b);
        adj[b].push_back(a);
    }

    const std::list<std::string>& neighbors(const std::string& symbol) const {
        static const std::list<std::string> empty;
        auto it = adj.find(symbol);
        return it == adj.end() ? empty : it->second;
    }

    // BFS out from `start` up to `maxHops`, returning each
    // reached symbol paired with its hop distance (1 = direct
    // neighbor, 2 = neighbor-of-neighbor, ...). Used by the
    // market engine to propagate a correlated price nudge that
    // decays with distance, and by the UI's "related stocks"
    // view.
    std::list<std::pair<std::string,int>> bfsWithinHops(const std::string& start, int maxHops) const {
        std::list<std::pair<std::string,int>> result;
        if (adj.find(start) == adj.end()) return result;

        std::unordered_set<std::string> visited;
        std::queue<std::pair<std::string,int>> q;
        q.push({start, 0});
        visited.insert(start);

        while (!q.empty()) {
            auto [sym, dist] = q.front();
            q.pop();
            if (dist > 0) result.push_back({sym, dist});
            if (dist >= maxHops) continue;
            for (const auto& n : neighbors(sym)) {
                if (!visited.count(n)) {
                    visited.insert(n);
                    q.push({n, dist + 1});
                }
            }
        }
        return result;
    }
};

#endif // STOCK_GRAPH_H
