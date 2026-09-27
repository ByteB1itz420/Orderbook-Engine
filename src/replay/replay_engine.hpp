#pragma once

#include <functional>
#include <vector>

#include "book/order_book.hpp"
#include "feed/events.hpp"

namespace lob {

// Drives the book from a recorded event stream, in timestamp order.
//
// Determinism contract (hard requirement from the spec): the same input
// file plus the same config must produce a byte-identical output log. That
// means no wall-clock reads, no hash-order iteration, and no unsequenced
// timestamps inside the engine.
class ReplayEngine {
  public:
    using TradeHandler = std::function<void(TimestampNs, Price, Quantity)>;

    ReplayEngine() = default;

    // Feed one event. Applies it to the book in receive order.
    void process(const Event& event);

    const OrderBook& book() const { return book_; }

    void setTradeHandler(TradeHandler handler) { on_trade_ = std::move(handler); }

  private:
    OrderBook book_;
    TradeHandler on_trade_;
};

}  // namespace lob
