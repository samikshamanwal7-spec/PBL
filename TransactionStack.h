#ifndef TRANSACTION_STACK_H
#define TRANSACTION_STACK_H

#include <string>
#include <list>
#include <ctime>

// ============================================================
// TransactionStack.h -- DSA CONCEPT: STACK (LIFO)
//
// Every completed BUY/SELL is pushed here. "Undo last
// transaction" pops the most recent one and reverses its
// effect on cash + holdings -- a textbook use of a stack,
// since only the most recent action can sensibly be undone.
// Implemented on top of std::list to show the LIFO access
// pattern explicitly (push_back/pop_back only -- no random
// access is used for stack semantics), while std::list
// itself is used purely as the underlying storage.
// ============================================================

enum class TxType { BUY, SELL };

struct Transaction {
    TxType type;
    std::string symbol;
    int quantity;
    double priceEach;
    double realizedPnL; // only meaningful for SELL
    std::time_t timestamp;

    double total() const { return priceEach * quantity; }
};

class TransactionStack {
private:
    std::list<Transaction> data; // used strictly as a stack

public:
    void push(const Transaction& t) { data.push_back(t); }

    bool isEmpty() const { return data.empty(); }

    // Removes and returns the top (most recent) transaction.
    Transaction pop() {
        Transaction t = data.back();
        data.pop_back();
        return t;
    }

    const Transaction& peek() const { return data.back(); }

    int size() const { return (int)data.size(); }

    // Oldest-first copy, used when saving the history to disk so that
    // loading it back (push in this order) rebuilds the same stack.
    std::list<Transaction> oldestFirst() const { return data; }

    // Read-only view, most recent first, for printing history.
    std::list<Transaction> recentFirst() const {
        std::list<Transaction> out(data.rbegin(), data.rend());
        return out;
    }
};

#endif // TRANSACTION_STACK_H
