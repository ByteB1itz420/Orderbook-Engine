#include "replay/replay_engine.hpp"

namespace lob {

void ReplayEngine::setTradeHandler(TradeHandler handler) {
    on_trade_ = std::move(handler);
    book_.setTradeHandler(on_trade_);
}

void ReplayEngine::process(const Event& event) {
    std::visit(
        [this](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, AddOrderEvent>) {
                book_.addObservedOrder(e.order);
            } else if constexpr (std::is_same_v<T, CancelOrderEvent>) {
                book_.cancelOrder(e.id);
            } else if constexpr (std::is_same_v<T, ReduceOrderEvent>) {
                book_.reduceOrder(e.id, e.quantity);
            } else if constexpr (std::is_same_v<T, ReplaceOrderEvent>) {
                book_.replaceOrder(e.id, e.new_price, e.new_quantity);
            } else if constexpr (std::is_same_v<T, ExecutionEvent>) {
                // Outbound feed execution: reduce the resting order only. It is
                // not an incoming command to our separate simulator.
                if (!e.hidden) book_.reduceOrder(e.id, e.quantity);
            }
        },
        event);
}

}  // namespace lob
