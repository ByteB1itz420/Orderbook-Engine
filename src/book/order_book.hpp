#pragma once

#include <map>
#include <optional>
#include <unordered_map>

#include "book/order.hpp"
#include "book/price_level.hpp"
#include "book/types.hpp"

namespace lob {

// A single-side view of the book: best bid / best ask plus aggregate depth.
struct TopOfBook {
    Price price{};
    Quantity quantity{};
};

// The limit order book for one symbol.
//
// Milestone 1 state: the container plumbing is implemented on the naive
// baseline structures (std::map of price -> PriceLevel, plus an id index).
// The matching loop is intentionally left as a marked TODO for the owner to
// write; see match() in order_book.cpp.
class OrderBook {
  public:
    OrderBook() = default;

    // Insert an order. Limit orders rest on the book after any matching.
    // Returns true if the id was new and the order was accepted.
    bool addOrder(const Order& order);

    // Cancel a resting order by id. Returns false if the id is not resting.
    bool cancelOrder(OrderId id);

    // Amend price and/or quantity of a resting order. Loses time priority
    // (implemented as cancel + re-add, the common exchange rule).
    bool replaceOrder(OrderId id, Price new_price, Quantity new_quantity);

    // O(1) top-of-book queries. std::nullopt when that side is empty.
    std::optional<TopOfBook> bestBid() const;
    std::optional<TopOfBook> bestAsk() const;

    // Resting depth on one side: number of price levels.
    std::size_t bidLevels() const { return bids_.size(); }
    std::size_t askLevels() const { return asks_.size(); }

    bool contains(OrderId id) const { return index_.find(id) != index_.end(); }
    std::size_t restingOrderCount() const { return index_.size(); }

  private:
    // Sweep the opposite side while `incoming` can fill. Emits trades.
    //
    // TODO(ayush): IMPLEMENT THE MATCHING LOOP. This is the heart of the
    // project. Price-time priority:
    //   1. While the incoming order has quantity left and the opposite side
    //      has a level that crosses its price (for Market: any level):
    //        - take the front order at the best opposite level (time priority)
    //        - fill min(incoming.quantity, resting.quantity) on both sides
    //        - record a trade (price of the RESTING order, aggressor = incoming)
    //        - pop fully filled resting orders; drop empty levels
    //   2. Self-trade prevention: if the resting order belongs to the same
    //      owner (needs an owner field) and STP mode is cancel-resting,
    //      cancel the resting order instead of filling. v1: skip owners.
    //   3. Whatever is left: Limit rests on the book; Ioc/Market is dropped.
    //   Write the invariant tests FIRST (tests/test_order_book_api.cpp has
    //   stubs): best bid < best ask, book equals rebuild from the event log.
    void match(Order& incoming);

    using Levels = std::map<Price, PriceLevel>;  // naive baseline structure

    Levels bids_;  // ascending by price; best bid is rbegin()
    Levels asks_;  // ascending by price; best ask is begin()

    // id -> (side, price) so cancel/replace find a resting order in O(log n)
    // without scanning. The optimized engine replaces this with pool pointers.
    std::unordered_map<OrderId, std::pair<Side, Price>> index_;

    static Levels& sideLevels(OrderBook& book, Side side) {
        return side == Side::Buy ? book.bids_ : book.asks_;
    }
};

}  // namespace lob
