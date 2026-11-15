#ifndef PRICE_HISTORY_H
#define PRICE_HISTORY_H

#include <string>
#include <vector>
#include <stdexcept>

// ============================================================
// PriceHistory.h  -- DSA CONCEPT: DOUBLY LINKED LIST
//
// Every stock keeps its own price history as a doubly linked
// list of PriceNode. Why a linked list instead of a vector?
//   - New ticks are appended in O(1) without reallocation.
//   - We can walk backward (prev) to compute moving trends,
//     or forward (next) to replay history, without needing
//     random access or contiguous memory.
//   - It cleanly demonstrates node-based structures with
//     dynamic pointers, which is the point of the exercise.
// ============================================================

struct PriceNode {
    double price;
    int tick;              // simulated time step
    PriceNode* prev;
    PriceNode* next;

    PriceNode(double p, int t)
        : price(p), tick(t), prev(nullptr), next(nullptr) {}
};

class PriceHistory {
private:
    PriceNode* head;   // oldest price
    PriceNode* tail;    // most recent price
    int count;

public:
    PriceHistory() : head(nullptr), tail(nullptr), count(0) {}

    ~PriceHistory() {
        PriceNode* cur = head;
        while (cur) {
            PriceNode* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
    }

    // Non-copyable (raw owning pointers) but movable is not needed
    // for this project's scope; disable copy to avoid double-free.
    PriceHistory(const PriceHistory&) = delete;
    PriceHistory& operator=(const PriceHistory&) = delete;

    void append(double price) {
        PriceNode* node = new PriceNode(price, count);
        if (!tail) {
            head = tail = node;
        } else {
            node->prev = tail;
            tail->next = node;
            tail = node;
        }
        count++;
    }

    double latest() const {
        if (!tail) throw std::runtime_error("PriceHistory is empty");
        return tail->price;
    }

    double previous() const {
        if (!tail || !tail->prev) return latest();
        return tail->prev->price;
    }

    int size() const { return count; }

    // Simple moving average over the last n ticks, walked
    // backwards from tail using the `prev` pointers.
    double movingAverage(int n) const {
        if (!tail) return 0.0;
        double sum = 0.0;
        int c = 0;
        PriceNode* cur = tail;
        while (cur && c < n) {
            sum += cur->price;
            cur = cur->prev;
            c++;
        }
        return c > 0 ? sum / c : 0.0;
    }

    // Returns the last n prices oldest-first (for charting).
    std::vector<double> lastN(int n) const {
        std::vector<double> out;
        PriceNode* cur = tail;
        int c = 0;
        while (cur && c < n) {
            out.push_back(cur->price);
            cur = cur->prev;
            c++;
        }
        // reverse to chronological order
        std::vector<double> chrono(out.rbegin(), out.rend());
        return chrono;
    }

    // Net percent change from the oldest kept tick to latest.
    double percentChangeSinceStart() const {
        if (!head || head == tail) return 0.0;
        if (head->price == 0.0) return 0.0;
        return ((tail->price - head->price) / head->price) * 100.0;
    }
};

#endif // PRICE_HISTORY_H
