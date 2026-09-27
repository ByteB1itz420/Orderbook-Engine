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
                book_.addOrder(e.order);
            } else if constexpr (std::is_same_v<T, CancelOrderEvent>) {
                book_.cancelOrder(e.id);
            } else if constexpr (std::is_same_v<T, ReduceOrderEvent>) {
                book_.reduceOrder(e.id, e.quantity);
            } else if constexpr (std::is_same_v<T, ReplaceOrderEvent>) {
                book_.replaceOrder(e.id, e.new_price, e.new_quantity);
            } else if constexpr (std::is_same_v<T, ExecutionEvent>) {
                // Feed-reported trade. A visible execution consumed part of
                // the resting order; a hidden one left the visible book
                // untouched. Compare these against on_trade_ output when
                // validating the matching loop against recorded truth.
                if (!e.hidden) book_.reduceOrder(e.id, e.quantity);
            }
        },
        event);
}

}  // namespace lob
