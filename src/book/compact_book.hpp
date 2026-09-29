#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "book/order.hpp"
#include "book/trade.hpp"

namespace lob {

// Fixed-capacity alternative to the readable OrderBook. All storage is
// allocated in the constructor. Capacity exhaustion rejects an operation.
// Price levels are a sorted intrusive list: finding a *new* level is O(L),
// cancelling a known order is O(1) expected after the flat-ID probe.
class CompactBook {
 public:
    explicit CompactBook(std::size_t max_orders = 65536, std::size_t max_levels = 2048);
    bool addOrder(const Order& order);
    bool cancelOrder(OrderId id);
    bool reduceOrder(OrderId id, Quantity amount);
    bool replaceOrder(OrderId id, Price price, Quantity quantity);
    std::size_t restingOrderCount() const { return live_; }
    bool contains(OrderId id) const { return lookup(id) != -1; }
    bool checkInvariants() const;
    void setTradeHandler(std::function<void(const Trade&)> handler) { on_trade_ = std::move(handler); }
    struct Top { Price price; Quantity quantity; };
    bool top(Side side, Top& out) const;
    std::size_t orderCapacity() const { return nodes_.size(); }
    std::size_t levelCapacity() const { return levels_.size(); }

 private:
    struct Node { Order order{}; int prev{-1}, next{-1}, level{-1}, free_next{-1}; bool live{false}; };
    struct Level { Price price{0}; Quantity quantity{0}; int head{-1}, tail{-1}, prev{-1}, next{-1}, free_next{-1}; bool live{false}; };
    struct Slot { OrderId id{0}; int node{-1}; enum State : std::uint8_t {Empty, Live, Deleted} state{Empty}; };
    std::vector<Node> nodes_;
    std::vector<Level> levels_;
    std::vector<Slot> slots_;
    int free_node_{-1}, free_level_{-1};
    int heads_[2]{-1,-1}; // buy sorted descending; sell ascending
    std::size_t live_{0};
    std::function<void(const Trade&)> on_trade_;

    static int sideIndex(Side side) { return side == Side::Buy ? 0 : 1; }
    static std::size_t hash(OrderId id);
    int lookup(OrderId id) const;
    int slotForInsert(OrderId id) const;
    void eraseId(OrderId id);
    int findLevel(Side side, Price price) const;
    int makeLevel(Side side, Price price);
    void retireLevel(Side side, int level);
    int takeNode();
    void releaseNode(int node);
    void append(int level, int node);
    void unlink(int node);
    void fillHead(int level, Quantity qty);
    void match(Order& incoming);
};
} // namespace lob
