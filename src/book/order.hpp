#pragma once

#include "book/types.hpp"

namespace lob {

enum class Side { Buy, Sell };

// The opposite side of the book, used when an incoming order sweeps the
// resting side. Buy sweeps Sell asks, Sell sweeps Buy bids.
constexpr Side opposite(Side side) {
    return side == Side::Buy ? Side::Sell : Side::Buy;
}

enum class OrderType {
    Limit,
    Market,
    Ioc,  // immediate-or-cancel: fill what you can now, cancel the rest
    // TODO(ayush, v2): FillOrKill
};

struct Order {
    OrderId id{};
    Side side{};
    OrderType type{OrderType::Limit};
    Price price{};       // ticks; ignored for Market orders
    Quantity quantity{}; // remaining (unfilled) quantity
    TimestampNs timestamp{};
};

}  // namespace lob
