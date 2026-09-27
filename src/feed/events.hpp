#pragma once

#include <variant>

#include "book/order.hpp"
#include "book/types.hpp"

namespace lob {

// The event types an exchange feed delivers, normalized so any parser
// (LOBSTER, ITCH-style, crypto L2/L3) can emit the same stream. The replay
// engine consumes this variant; nothing downstream knows which venue the
// data came from.

struct AddOrderEvent {
    TimestampNs ts{};
    Order order{};
};

struct CancelOrderEvent {
    TimestampNs ts{};
    OrderId id{};
};

struct ReplaceOrderEvent {
    TimestampNs ts{};
    OrderId id{};
    Price new_price{};
    Quantity new_quantity{};
};

// A trade reported by the feed itself (e.g. LOBSTER execution messages).
// Distinct from trades our own matching loop produces; the replay engine
// uses these to validate its own output against the recorded truth.
struct ExecutionEvent {
    TimestampNs ts{};
    OrderId id{};
    Price price{};
    Quantity quantity{};
};

using Event = std::variant<AddOrderEvent, CancelOrderEvent, ReplaceOrderEvent, ExecutionEvent>;

}  // namespace lob
