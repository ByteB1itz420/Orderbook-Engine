#pragma once

#include <cstddef>
#include <vector>

#include "book/order.hpp"

namespace lob {

// Preallocated slab pool for orders. The engine never allocates on the hot
// path after startup: orders come from here and are returned here.
//
// Milestone 1 version: the pool grows in slabs if exhausted (startup-only
// cost, amortized away from steady state). A later milestone makes the hot
// path strictly non-allocating and reports pool high-water marks.
class OrderPool {
  public:
    explicit OrderPool(std::size_t initial_capacity = 1 << 16) { grow(initial_capacity); }

    OrderPool(const OrderPool&) = delete;
    OrderPool& operator=(const OrderPool&) = delete;

    // Take a slot. The returned reference stays valid until release().
    Order& acquire() {
        if (free_.empty()) grow(slabs_.size() * slabSize());
        Order* slot = free_.back();
        free_.pop_back();
        *slot = Order{};
        return *slot;
    }

    // Return a slot to the pool.
    void release(Order& order) { free_.push_back(&order); }

    std::size_t capacity() const { return slabs_.size() * slabSize(); }
    std::size_t freeCount() const { return free_.size(); }

  private:
    static constexpr std::size_t slabSize() { return 1 << 16; }

    void grow(std::size_t /*hint*/) {
        // One slab at a time; slab storage never moves, so Order* stays valid.
        slabs_.emplace_back(slabSize());
        for (Order& order : slabs_.back()) free_.push_back(&order);
    }

    std::vector<std::vector<Order>> slabs_;
    std::vector<Order*> free_;
};

}  // namespace lob
