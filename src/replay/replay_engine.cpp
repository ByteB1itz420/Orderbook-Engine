#include "replay/replay_engine.hpp"

namespace lob {

void ReplayEngine::process(const Event& event) {
    std::visit(
        [this](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, AddOrderEvent>) {
                book_.addOrder(e.order);
            } else if constexpr (std::is_same_v<T, CancelOrderEvent>) {
                book_.cancelOrder(e.id);
            } else if constexpr (std::is_same_v<T, ReplaceOrderEvent>) {
                book_.replaceOrder(e.id, e.new_price, e.new_quantity);
            } else if constexpr (std::is_same_v<T, ExecutionEvent>) {
                // Feed-reported trade. TODO(ayush): once match() exists,
                // compare these against the engine's own trades to validate
                // the matching loop against recorded truth.
            }
        },
        event);
}

}  // namespace lob
