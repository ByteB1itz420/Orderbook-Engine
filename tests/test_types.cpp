#include <catch2/catch_test_macros.hpp>

#include "book/order.hpp"

using lob::Order;
using lob::OrderType;
using lob::Side;

TEST_CASE("opposite side", "[types]") {
    REQUIRE(lob::opposite(Side::Buy) == Side::Sell);
    REQUIRE(lob::opposite(Side::Sell) == Side::Buy);
}

TEST_CASE("order defaults", "[types]") {
    Order order{};
    REQUIRE(order.type == OrderType::Limit);
    REQUIRE(order.quantity == 0);
    REQUIRE(order.price == 0);
}
