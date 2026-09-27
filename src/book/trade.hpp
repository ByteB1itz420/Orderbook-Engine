#pragma once

#include "book/order.hpp"
#include "book/types.hpp"

namespace lob {

// One fill produced by the matching loop. The trade price is always the
// RESTING order's price: the aggressor pays/earns the price it walked into.
struct Trade {
    TimestampNs ts{};
    Price price{};
    Quantity quantity{};
    Side aggressor{};      // side of the incoming order
    OrderId buyOrderId{};  // resting or incoming, whichever bought
    OrderId sellOrderId{};
};

}  // namespace lob
