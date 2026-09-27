#pragma once

#include <functional>

#include "book/order_book.hpp"
#include "book/trade.hpp"
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
    using TradeHandler = std::function<void(const Trade&)>;

    ReplayEngine() = default;

    // Feed one event. Applies it to the book in receive order.
    void process(const Event& event);

    const OrderBook& book() const { return book_; }
    OrderBook& book() { return book_; }

    // Called for every trade the matching loop produces. Feed-reported
    // executions (ExecutionEvent) do NOT fire this; they are the recorded
    // truth to validate against, not our own output.
    void setTradeHandler(TradeHandler handler);

  private:
    OrderBook book_;
    TradeHandler on_trade_;
};

}  // namespace lob
