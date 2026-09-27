#include <catch2/catch_test_macros.hpp>

#include "book/order_book.hpp"

using lob::Order;
using lob::OrderBook;
using lob::OrderType;
using lob::Side;

namespace {
Order limit(lob::OrderId id, Side side, lob::Price price, lob::Quantity qty) {
    Order order{};
    order.id = id;
    order.side = side;
    order.type = OrderType::Limit;
    order.price = price;
    order.quantity = qty;
    return order;
}
}  // namespace

TEST_CASE("empty book has no top of book", "[book]") {
    OrderBook book;
    REQUIRE_FALSE(book.bestBid().has_value());
    REQUIRE_FALSE(book.bestAsk().has_value());
}

TEST_CASE("resting orders appear at best bid/ask", "[book]") {
    OrderBook book;
    book.addOrder(limit(1, Side::Buy, 10000, 10));
    book.addOrder(limit(2, Side::Buy, 10050, 5));
    book.addOrder(limit(3, Side::Sell, 10100, 7));

    // NOTE: until match() is implemented, every limit order rests. Once the
    // matching loop exists, an order at 10050 bid against a 10100 ask still
    // rests (no cross), so this test stays valid.
    REQUIRE(book.bestBid()->price == 10050);
    REQUIRE(book.bestBid()->quantity == 5);
    REQUIRE(book.bestAsk()->price == 10100);
    REQUIRE(book.restingOrderCount() == 3);
}

TEST_CASE("cancel removes the order and cleans empty levels", "[book]") {
    OrderBook book;
    book.addOrder(limit(1, Side::Buy, 10000, 10));
    book.addOrder(limit(2, Side::Buy, 10050, 5));

    REQUIRE(book.cancelOrder(2));
    REQUIRE(book.bestBid()->price == 10000);
    REQUIRE(book.bidLevels() == 1);
    REQUIRE_FALSE(book.cancelOrder(2));  // already gone
}

TEST_CASE("replace loses time priority and moves level", "[book]") {
    OrderBook book;
    book.addOrder(limit(1, Side::Sell, 10100, 10));
    book.addOrder(limit(2, Side::Sell, 10100, 20));

    REQUIRE(book.replaceOrder(1, 10200, 15));
    REQUIRE(book.bestAsk()->price == 10100);
    REQUIRE(book.bestAsk()->quantity == 20);
    REQUIRE(book.askLevels() == 2);
}

TEST_CASE("duplicate ids are rejected", "[book]") {
    OrderBook book;
    REQUIRE(book.addOrder(limit(1, Side::Buy, 10000, 10)));
    REQUIRE_FALSE(book.addOrder(limit(1, Side::Buy, 10000, 10)));
    REQUIRE(book.restingOrderCount() == 1);
}

// ---------------------------------------------------------------------------
// TODO(ayush): matching tests. Write these BEFORE writing match(). Suggested:
//   - a crossing limit order fills at the RESTING price, not its own
//   - FIFO: two resting orders at one level fill in arrival order
//   - partial fill leaves the remainder resting
//   - Ioc fills what it can and drops the rest; Market sweeps multiple levels
//   - invariant: after any sequence, best bid < best ask
//   - invariant: the live book equals a rebuild from the event log
// ---------------------------------------------------------------------------
