#include <algorithm>
#include <random>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "book/compact_book.hpp"
#include "book/order_book.hpp"
using namespace lob;

TEST_CASE("compact matches baseline on seeded mixed traffic", "[compact]") {
    OrderBook baseline;
    CompactBook compact(8192, 128);
    std::vector<Trade> a, b;
    baseline.setTradeHandler([&](const Trade& t){a.push_back(t);});
    compact.setTradeHandler([&](const Trade& t){b.push_back(t);});
    std::mt19937 rng(913201);
    std::vector<OrderId> ids;
    for (OrderId id = 1; id <= 5000; ++id) {
        if (id % 3 == 0 && !ids.empty()) {
            auto at = static_cast<std::size_t>(rng() % ids.size());
            OrderId old = ids[at];
            REQUIRE(baseline.cancelOrder(old) == compact.cancelOrder(old));
            ids.erase(ids.begin() + static_cast<std::ptrdiff_t>(at));
        } else if (id % 7 == 0 && !ids.empty()) {
            auto at = static_cast<std::size_t>(rng() % ids.size());
            OrderId old = ids[at];
            REQUIRE(baseline.reduceOrder(old, 1) == compact.reduceOrder(old, 1));
        } else {
            Order o{}; o.id = id; o.side = (rng()%2) ? Side::Buy : Side::Sell;
            o.type = (id % 17 == 0) ? OrderType::Market : OrderType::Limit;
            o.price = 9900 + static_cast<Price>(rng()%21)*10;
            o.quantity = 1 + static_cast<Quantity>(rng()%40);
            REQUIRE(baseline.addOrder(o) == compact.addOrder(o));
            if (compact.contains(id)) ids.push_back(id);
        }
        CompactBook::Top top{};
        for (auto side : {Side::Buy,Side::Sell}) {
            auto x = side == Side::Buy ? baseline.bestBid() : baseline.bestAsk();
            REQUIRE(compact.top(side,top) == x.has_value());
            if (x) { REQUIRE(top.price == x->price); REQUIRE(top.quantity == x->quantity); }
        }
        REQUIRE(compact.restingOrderCount() == baseline.restingOrderCount());
        REQUIRE(compact.checkInvariants());
        REQUIRE(a.size() == b.size());
        if (!a.empty()) {
            REQUIRE(a.back().price == b.back().price);
            REQUIRE(a.back().quantity == b.back().quantity);
            REQUIRE(a.back().buyOrderId == b.back().buyOrderId);
            REQUIRE(a.back().sellOrderId == b.back().sellOrderId);
        }
    }
}

TEST_CASE("compact rejects capacity without mutating", "[compact]") {
    CompactBook book(1,1); Order o{}; o.id=1; o.side=Side::Buy;
    o.price=100; o.quantity=10;
    REQUIRE(book.addOrder(o));
    o.id=2; o.price=110;
    REQUIRE_FALSE(book.addOrder(o));
    CompactBook::Top top{};
    REQUIRE(book.top(Side::Buy,top)); REQUIRE(top.price==100); REQUIRE(top.quantity==10);
    REQUIRE(book.checkInvariants());
}
