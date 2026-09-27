#include <catch2/catch_test_macros.hpp>

#include "book/order_pool.hpp"

using lob::OrderPool;

TEST_CASE("pool hands out zeroed orders", "[pool]") {
    OrderPool pool;
    lob::Order& order = pool.acquire();
    REQUIRE(order.id == 0);
    REQUIRE(order.quantity == 0);
    REQUIRE(pool.freeCount() == pool.capacity() - 1);
}

TEST_CASE("released slots are reused", "[pool]") {
    OrderPool pool;
    const std::size_t before = pool.freeCount();
    lob::Order& a = pool.acquire();
    lob::Order& b = pool.acquire();
    pool.release(a);
    pool.release(b);
    REQUIRE(pool.freeCount() == before);
}
