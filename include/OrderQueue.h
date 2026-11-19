#ifndef ORDER_QUEUE_H
#define ORDER_QUEUE_H

#include <string>
#include <deque>
#include <memory>
#include <iostream>

// ============================================================
// OrderQueue.h
// DSA CONCEPT: QUEUE (FIFO)          -- order execution engine
// OOP CONCEPT: INHERITANCE + POLYMORPHISM -- Order hierarchy
//
// When the user places a buy/sell, it isn't applied instantly:
// it's enqueued as a pending Order. The market engine then
// processes the queue in FIFO order (first order placed is the
// first one executed against the current market price), the
// same way a real exchange's order book works at a basic level.
//
// Order is an abstract base class; BuyOrder and SellOrder
// override execute() -- classic runtime polymorphism.
// ============================================================

class Portfolio; // forward declaration, defined in Portfolio.h

class Order {
protected:
    std::string symbol;
    int quantity;

public:
    Order(std::string sym, int qty) : symbol(std::move(sym)), quantity(qty) {}
    virtual ~Order() = default;

    const std::string& getSymbol() const { return symbol; }
    int getQuantity() const { return quantity; }

    // Executes against the portfolio at the given market price.
    // Returns true if the order succeeded, false if rejected
    // (e.g. insufficient funds or shares).
    virtual bool execute(Portfolio& portfolio, double marketPrice, std::string& message) = 0;
    virtual std::string label() const = 0;
};

class BuyOrder : public Order {
public:
    BuyOrder(std::string sym, int qty) : Order(std::move(sym), qty) {}
    bool execute(Portfolio& portfolio, double marketPrice, std::string& message) override;
    std::string label() const override { return "BUY"; }
};

class SellOrder : public Order {
public:
    SellOrder(std::string sym, int qty) : Order(std::move(sym), qty) {}
    bool execute(Portfolio& portfolio, double marketPrice, std::string& message) override;
    std::string label() const override { return "SELL"; }
};

// FIFO queue of pending orders, built on std::deque but only
// ever used through push_back (enqueue) / pop_front (dequeue)
// / front (peek) to keep strict queue semantics.
class OrderQueue {
private:
    std::deque<std::shared_ptr<Order>> data;

public:
    void enqueue(std::shared_ptr<Order> order) { data.push_back(order); }

    bool isEmpty() const { return data.empty(); }

    std::shared_ptr<Order> dequeue() {
        auto front = data.front();
        data.pop_front();
        return front;
    }

    int size() const { return (int)data.size(); }
};

#endif // ORDER_QUEUE_H
