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

// Partial cancellation: reduce the resting order by `quantity` (LOBSTER
// type 2). Full deletes come through as CancelOrderEvent.
struct ReduceOrderEvent {
    TimestampNs ts{};
    OrderId id{};
    Quantity quantity{};
};

struct ReplaceOrderEvent {
    TimestampNs ts{};
    OrderId id{};
    Price new_price{};
    Quantity new_quantity{};
};

// A trade reported by the feed itself (LOBSTER type 4/5). In LOBSTER the
// aggressive order never appears as an add: you only see its effect on the
// resting order. So a visible execution reduces the resting order, and a
// hidden one does not touch the visible book. These are also the recorded
// truth a matching engine can validate its own trades against.
struct ExecutionEvent {
    TimestampNs ts{};
    OrderId id{};
    Price price{};
    Quantity quantity{};
    bool hidden{};  // true = hidden execution, no visible-book effect
};

using Event = std::variant<AddOrderEvent, CancelOrderEvent, ReduceOrderEvent, ReplaceOrderEvent,
                           ExecutionEvent>;

}  // namespace lob
