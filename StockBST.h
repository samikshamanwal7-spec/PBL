#ifndef STOCK_BST_H
#define STOCK_BST_H

#include <memory>
#include <list>
#include <string>
#include "modules/stock_market/Stock.h"

// ============================================================
// StockBST.h -- DSA CONCEPT: BINARY SEARCH TREE
//
// The market holds many stocks. Instead of scanning a list
// every time we look one up by symbol, we index every Stock*
// in a BST keyed by symbol (alphabetical). This gives O(log n)
// average search, and a free-by-product: an in-order traversal
// yields all stocks sorted alphabetically for listing.
//
// The tree stores non-owning raw pointers to Stock objects
// that are actually owned by the Market (see Market.h); the
// BST is purely an index structure, not the owner.
// ============================================================

struct BSTNode {
    Stock* stock;
    std::unique_ptr<BSTNode> left;
    std::unique_ptr<BSTNode> right;
    explicit BSTNode(Stock* s) : stock(s) {}
};

class StockBST {
private:
    std::unique_ptr<BSTNode> root;

    void insertNode(std::unique_ptr<BSTNode>& node, Stock* s) {
        if (!node) {
            node = std::make_unique<BSTNode>(s);
            return;
        }
        if (s->getSymbol() < node->stock->getSymbol())
            insertNode(node->left, s);
        else if (s->getSymbol() > node->stock->getSymbol())
            insertNode(node->right, s);
        // equal symbol: ignore duplicate
    }

    Stock* findNode(BSTNode* node, const std::string& symbol) const {
        if (!node) return nullptr;
        if (symbol == node->stock->getSymbol()) return node->stock;
        if (symbol < node->stock->getSymbol()) return findNode(node->left.get(), symbol);
        return findNode(node->right.get(), symbol);
    }

    void inorderCollect(BSTNode* node, std::list<Stock*>& out) const {
        if (!node) return;
        inorderCollect(node->left.get(), out);
        out.push_back(node->stock);
        inorderCollect(node->right.get(), out);
    }

public:
    void insert(Stock* s) { insertNode(root, s); }

    // O(log n) average-case lookup by ticker symbol.
    Stock* find(const std::string& symbol) const { return findNode(root.get(), symbol); }

    // All stocks in alphabetical order (in-order traversal).
    std::list<Stock*> allSortedBySymbol() const {
        std::list<Stock*> out;
        inorderCollect(root.get(), out);
        return out;
    }
};

#endif // STOCK_BST_H
