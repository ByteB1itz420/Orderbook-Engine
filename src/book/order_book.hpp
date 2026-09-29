#pragma once

#include <functional>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>
#include <cstddef>

#include "book/order.hpp"
#include "book/price_level.hpp"
#include "book/trade.hpp"
#include "book/types.hpp"

namespace lob {

// A single-side view of the book: best price plus aggregate resting quantity.
struct TopOfBook {
    Price price{};
    Quantity quantity{};
};

// Self-trade prevention. When an incoming order would match a resting order
// from the same owner:
//   None:          trade anyway (some venues allow it)
//   CancelResting: cancel the resting order, keep processing the incoming one
enum class SelfTradeMode { None, CancelResting };

// The limit order book for one symbol.
//
// Matching rule (strict price-time priority):
//   - the best price trades first; within one price, the oldest order trades
//     first (FIFO per PriceLevel)
//   - trades print at the RESTING order's price
//   - Limit leftovers rest; Market and IOC leftovers are dropped
//
// The container layer is the deliberate naive baseline (std::map of price ->
// PriceLevel plus an id index). The optimized engine is benchmarked against
// this baseline in a later milestone.
class OrderBook {
  public:
    using TradeHandler = std::function<void(const Trade&)>;

    OrderBook() = default;

    void setTradeHandler(TradeHandler handler) { on_trade_ = std::move(handler); }
    void setSelfTradeMode(SelfTradeMode mode) { stp_ = mode; }

    // Insert an order. Matching runs first; a Limit remainder rests on the
    // book, a Market/IOC remainder is dropped. Returns false for a duplicate
    // id or non-positive quantity.
    bool addOrder(const Order& order);

    // Historical outbound feed: record a visible resting order without matching.
    // The aggressor was already processed by the venue, not supplied here.
    bool addObservedOrder(const Order& order);

    struct LevelView { Price price; Quantity quantity; std::size_t orders; };
    std::vector<LevelView> depth(Side side, std::size_t limit) const;
    bool checkInvariants() const;

    // Cancel a resting order by id. Returns false if the id is not resting.
    bool cancelOrder(OrderId id);

    // Reduce a resting order's quantity (partial cancel / feed-reported
    // execution). Removes the order when the reduction covers it.
    bool reduceOrder(OrderId id, Quantity delta);

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
    // Sweep the opposite side while `incoming` can still fill. Emits a Trade
    // per fill through on_trade_. See the class comment for the rules.
    void match(Order& incoming);

    using Levels = std::map<Price, PriceLevel>;  // naive baseline structure

    Levels bids_;  // ascending by price; best bid is rbegin()
    Levels asks_;  // ascending by price; best ask is begin()

    // id -> (side, price) so cancel/replace find a resting order in O(log n)
    // without scanning. The optimized engine replaces this with pool pointers.
    std::unordered_map<OrderId, std::pair<Side, Price>> index_;

    TradeHandler on_trade_;
    SelfTradeMode stp_{SelfTradeMode::None};

    static Levels& sideLevels(OrderBook& book, Side side) {
        return side == Side::Buy ? book.bids_ : book.asks_;
    }
};

}  // namespace lob
