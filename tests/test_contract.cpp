#include <catch2/catch_test_macros.hpp>
#include "book/order_book.hpp"
#include "replay/replay_engine.hpp"

using namespace lob;

TEST_CASE("invalid replace does not delete resting order", "[contract]") {
    OrderBook book;
    Order o{}; o.id = 1; o.side = Side::Buy; o.type = OrderType::Limit;
    o.price = 100; o.quantity = 10;
    REQUIRE(book.addOrder(o));
    REQUIRE_FALSE(book.replaceOrder(1, 101, 0));
    REQUIRE(book.contains(1));
    REQUIRE(book.bestBid()->quantity == 10);
    REQUIRE(book.checkInvariants());
}

TEST_CASE("outbound feed add is not a new incoming order", "[feed]") {
    ReplayEngine replay;
    Order sell{}; sell.id = 1; sell.side = Side::Sell;
    sell.price = 100; sell.quantity = 10;
    Order buy{}; buy.id = 2; buy.side = Side::Buy;
    buy.price = 101; buy.quantity = 5;
    int simulated_trades = 0;
    replay.setTradeHandler([&](const Trade&) { ++simulated_trades; });
    replay.process(AddOrderEvent{1, sell});
    replay.process(AddOrderEvent{2, buy});
    REQUIRE(simulated_trades == 0);
    REQUIRE(replay.book().contains(1));
    REQUIRE(replay.book().contains(2));
    REQUIRE(replay.book().depth(Side::Sell, 1).front().quantity == 10);
    replay.process(ExecutionEvent{3, 1, 100, 5, false});
    REQUIRE(replay.book().depth(Side::Sell, 1).front().quantity == 5);
}
