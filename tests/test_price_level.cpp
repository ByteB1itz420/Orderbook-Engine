#include <catch2/catch_test_macros.hpp>

#include "book/price_level.hpp"

using lob::Order;
using lob::OrderType;
using lob::PriceLevel;
using lob::Side;

namespace {
Order makeOrder(lob::OrderId id, lob::Quantity qty, lob::Price price = 10000) {
    Order order{};
    order.id = id;
    order.side = Side::Buy;
    order.type = OrderType::Limit;
    order.price = price;
    order.quantity = qty;
    return order;
}
}  // namespace

TEST_CASE("price level is FIFO (time priority)", "[price_level]") {
    PriceLevel level(10000);
    level.pushBack(makeOrder(1, 10));
    level.pushBack(makeOrder(2, 20));
    level.pushBack(makeOrder(3, 30));

    REQUIRE(level.orderCount() == 3);
    REQUIRE(level.totalQuantity() == 60);
    REQUIRE(level.front().id == 1);  // first in, first out
}

TEST_CASE("fillFront partially fills then pops", "[price_level]") {
    PriceLevel level(10000);
    level.pushBack(makeOrder(1, 10));
    level.pushBack(makeOrder(2, 20));

    REQUIRE(level.fillFront(4) == 1);
    REQUIRE(level.front().id == 1);
    REQUIRE(level.front().quantity == 6);

    REQUIRE(level.fillFront(6) == 1);  // order 1 fully filled, popped
    REQUIRE(level.front().id == 2);
    REQUIRE(level.totalQuantity() == 20);
}

TEST_CASE("remove by id", "[price_level]") {
    PriceLevel level(10000);
    level.pushBack(makeOrder(1, 10));
    level.pushBack(makeOrder(2, 20));

    REQUIRE(level.remove(1));
    REQUIRE_FALSE(level.remove(99));
    REQUIRE(level.front().id == 2);
}
