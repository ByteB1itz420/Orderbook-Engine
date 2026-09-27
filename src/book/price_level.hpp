#pragma once

#include <cassert>
#include <deque>

#include "book/order.hpp"
#include "book/types.hpp"

namespace lob {

// One price level: the queue of resting orders at a single price.
// Orders sit in FIFO (time-priority) order; the front of the queue is the
// next order to be matched at this price.
//
// This is the naive baseline implementation (std::deque). It stays in the
// repo as the benchmark baseline; the optimized version is a later milestone.
class PriceLevel {
  public:
    explicit PriceLevel(Price price) : price_(price) {}

    Price price() const { return price_; }
    bool empty() const { return queue_.empty(); }
    std::size_t orderCount() const { return queue_.size(); }

    // Total resting quantity across all orders at this level.
    Quantity totalQuantity() const {
        Quantity total = 0;
        for (const Order& order : queue_) total += order.quantity;
        return total;
    }

    // Append a resting order at the back of the time-priority queue.
    void pushBack(const Order& order) {
        assert(order.price == price_);
        queue_.push_back(order);
    }

    // Remove an order by id. O(n) at this level; the optimized engine keeps an
    // id -> iterator index so this is O(1). Fine for the baseline.
    bool remove(OrderId id) {
        for (auto it = queue_.begin(); it != queue_.end(); ++it) {
            if (it->id == id) {
                queue_.erase(it);
                return true;
            }
        }
        return false;
    }

    Order& front() { return queue_.front(); }
    const Order& front() const { return queue_.front(); }

    // Fill the front order by `filled` quantity, popping it when fully filled.
    // Returns the order id that was (partially or fully) filled.
    OrderId fillFront(Quantity filled) {
        assert(!queue_.empty());
        Order& head = queue_.front();
        assert(filled > 0 && filled <= head.quantity);
        head.quantity -= filled;
        const OrderId id = head.id;
        if (head.quantity == 0) queue_.pop_front();
        return id;
    }

  private:
    Price price_;
    std::deque<Order> queue_;
};

}  // namespace lob
