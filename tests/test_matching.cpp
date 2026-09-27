#include <map>
#include <random>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "book/order_book.hpp"

using lob::Order;
using lob::OrderBook;
using lob::OrderType;
using lob::SelfTradeMode;
using lob::Side;
using lob::Trade;

namespace {
Order makeOrder(lob::OrderId id, Side side, OrderType type, lob::Price price,
                lob::Quantity qty, std::uint64_t owner = 0) {
    Order order{};
    order.id = id;
    order.side = side;
    order.type = type;
    order.price = price;
    order.quantity = qty;
    order.owner = owner;
    return order;
}
Order limit(lob::OrderId id, Side side, lob::Price price, lob::Quantity qty,
            std::uint64_t owner = 0) {
    return makeOrder(id, side, OrderType::Limit, price, qty, owner);
}
}  // namespace

TEST_CASE("a crossing limit fills at the RESTING price", "[match]") {
    OrderBook book;
    std::vector<Trade> trades;
    book.setTradeHandler([&](const Trade& t) { trades.push_back(t); });

    book.addOrder(limit(1, Side::Sell, 10100, 100));
    book.addOrder(limit(2, Side::Buy, 10200, 40));  // willing to pay up to 10200

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].price == 10100);  // but pays the resting 10100
    REQUIRE(trades[0].quantity == 40);
    REQUIRE(trades[0].aggressor == Side::Buy);
    REQUIRE(trades[0].buyOrderId == 2);
    REQUIRE(trades[0].sellOrderId == 1);
    REQUIRE(book.bestAsk()->quantity == 60);  // remainder stays resting
    REQUIRE_FALSE(book.bestBid().has_value());
}

TEST_CASE("time priority: the oldest order at a level fills first", "[match]") {
    OrderBook book;
    std::vector<Trade> trades;
    book.setTradeHandler([&](const Trade& t) { trades.push_back(t); });

    book.addOrder(limit(1, Side::Sell, 10100, 10));
    book.addOrder(limit(2, Side::Sell, 10100, 20));
    book.addOrder(limit(3, Side::Buy, 10100, 15));

    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].sellOrderId == 1);  // oldest first
    REQUIRE(trades[0].quantity == 10);
    REQUIRE(trades[1].sellOrderId == 2);
    REQUIRE(trades[1].quantity == 5);
    REQUIRE(book.bestAsk()->quantity == 15);  // order 2 keeps the rest
    REQUIRE(book.contains(2));
    REQUIRE_FALSE(book.contains(1));
}

TEST_CASE("partial fill: the aggressor's remainder rests", "[match]") {
    OrderBook book;
    book.addOrder(limit(1, Side::Sell, 10100, 100));
    book.addOrder(limit(2, Side::Buy, 10100, 150));

    REQUIRE(book.bestBid()->price == 10100);
    REQUIRE(book.bestBid()->quantity == 50);
    REQUIRE_FALSE(book.bestAsk().has_value());
    REQUIRE(book.contains(2));
}

TEST_CASE("IOC fills what it can and drops the rest", "[match]") {
    OrderBook book;
    std::vector<Trade> trades;
    book.setTradeHandler([&](const Trade& t) { trades.push_back(t); });

    book.addOrder(limit(1, Side::Sell, 10100, 100));
    book.addOrder(makeOrder(2, Side::Buy, OrderType::Ioc, 10100, 150));

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].quantity == 100);
    REQUIRE_FALSE(book.contains(2));  // leftover died, never rested
    REQUIRE_FALSE(book.bestBid().has_value());
    REQUIRE_FALSE(book.bestAsk().has_value());
}

TEST_CASE("a market order sweeps multiple levels", "[match]") {
    OrderBook book;
    std::vector<Trade> trades;
    book.setTradeHandler([&](const Trade& t) { trades.push_back(t); });

    book.addOrder(limit(1, Side::Sell, 10100, 50));
    book.addOrder(limit(2, Side::Sell, 10200, 50));
    book.addOrder(makeOrder(3, Side::Buy, OrderType::Market, 0, 80));

    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].price == 10100);  // best price first
    REQUIRE(trades[0].quantity == 50);
    REQUIRE(trades[1].price == 10200);
    REQUIRE(trades[1].quantity == 30);
    REQUIRE(book.bestAsk()->price == 10200);
    REQUIRE(book.bestAsk()->quantity == 20);
    REQUIRE_FALSE(book.contains(1));
}

TEST_CASE("a market order on an empty side just dies", "[match]") {
    OrderBook book;
    std::vector<Trade> trades;
    book.setTradeHandler([&](const Trade& t) { trades.push_back(t); });

    book.addOrder(makeOrder(1, Side::Buy, OrderType::Market, 0, 100));
    REQUIRE(trades.empty());
    REQUIRE_FALSE(book.contains(1));
    REQUIRE_FALSE(book.bestBid().has_value());
}

TEST_CASE("a non-crossing limit rests without trading", "[match]") {
    OrderBook book;
    std::vector<Trade> trades;
    book.setTradeHandler([&](const Trade& t) { trades.push_back(t); });

    book.addOrder(limit(1, Side::Sell, 10100, 100));
    book.addOrder(limit(2, Side::Buy, 10050, 50));

    REQUIRE(trades.empty());
    REQUIRE(book.bestBid()->price == 10050);
    REQUIRE(book.bestAsk()->price == 10100);
    REQUIRE(book.bestBid()->price < book.bestAsk()->price);  // invariant
}

TEST_CASE("STP cancel-resting: same-owner orders never trade", "[match][stp]") {
    OrderBook book;
    book.setSelfTradeMode(SelfTradeMode::CancelResting);
    std::vector<Trade> trades;
    book.setTradeHandler([&](const Trade& t) { trades.push_back(t); });

    book.addOrder(limit(1, Side::Sell, 10100, 100, /*owner=*/7));
    book.addOrder(limit(2, Side::Sell, 10100, 50, /*owner=*/8));
    book.addOrder(limit(3, Side::Buy, 10100, 30, /*owner=*/7));

    // Owner 7's resting order is cancelled instead of filled; the buy then
    // trades against owner 8's order at the same level.
    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].sellOrderId == 2);
    REQUIRE(trades[0].quantity == 30);
    REQUIRE_FALSE(book.contains(1));  // cancelled by STP
    REQUIRE(book.bestAsk()->quantity == 20);
}

TEST_CASE("without STP, same-owner orders trade normally", "[match][stp]") {
    OrderBook book;  // default: SelfTradeMode::None
    std::vector<Trade> trades;
    book.setTradeHandler([&](const Trade& t) { trades.push_back(t); });

    book.addOrder(limit(1, Side::Sell, 10100, 100, /*owner=*/7));
    book.addOrder(limit(2, Side::Buy, 10100, 40, /*owner=*/7));

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].quantity == 40);
    REQUIRE(book.bestAsk()->quantity == 60);
}

// Differential test: random passive (never crossing) traffic must reproduce a
// plain std::map model exactly. Crossing behavior is covered above; here the
// goal is container bookkeeping under thousands of random ops.
TEST_CASE("randomized passive traffic matches a simple model", "[match][invariant]") {
    std::mt19937_64 rng(20260927);
    OrderBook book;

    // Model: side -> price -> total quantity, plus id -> (side, price, qty).
    std::map<lob::Price, lob::Quantity> model_bids, model_asks;
    struct Live {
        Side side;
        lob::Price price;
        lob::Quantity qty;
    };
    std::map<lob::OrderId, Live> live;

    auto model_side = [&](Side s) -> auto& { return s == Side::Buy ? model_bids : model_asks; };
    std::uniform_int_distribution<int> op_dist(0, 99);
    std::uniform_int_distribution<lob::Price> bid_px(9800, 10000);
    std::uniform_int_distribution<lob::Price> ask_px(10001, 10200);
    std::uniform_int_distribution<lob::Quantity> qty_dist(1, 100);

    lob::OrderId next_id = 1;
    for (int step = 0; step < 5000; ++step) {
        const int op = op_dist(rng);
        if (op < 60 || live.empty()) {  // add
            const Side side = (rng() % 2 == 0) ? Side::Buy : Side::Sell;
            const lob::Price price = (side == Side::Buy) ? bid_px(rng) : ask_px(rng);
            const lob::Quantity qty = qty_dist(rng);
            REQUIRE(book.addOrder(limit(next_id, side, price, qty)));
            live[next_id] = Live{side, price, qty};
            model_side(side)[price] += qty;
            ++next_id;
        } else if (op < 80) {  // cancel
            auto it = live.begin();
            std::advance(it, rng() % live.size());
            const auto [id, info] = *it;
            REQUIRE(book.cancelOrder(id));
            if ((model_side(info.side)[info.price] -= info.qty) == 0)
                model_side(info.side).erase(info.price);
            live.erase(it);
        } else {  // reduce (partial cancel)
            auto it = live.begin();
            std::advance(it, rng() % live.size());
            const lob::Quantity delta = qty_dist(rng);
            const auto id = it->first;
            Live& info = it->second;
            REQUIRE(book.reduceOrder(id, delta));
            auto& px_qty = model_side(info.side)[info.price];
            if (delta >= info.qty) {
                px_qty -= info.qty;
                if (px_qty == 0) model_side(info.side).erase(info.price);
                live.erase(it);
            } else {
                info.qty -= delta;
                px_qty -= delta;
            }
        }

        if (step % 100 == 0) {
            REQUIRE(book.restingOrderCount() == live.size());
            if (model_bids.empty()) {
                REQUIRE_FALSE(book.bestBid().has_value());
            } else {
                REQUIRE(book.bestBid()->price == model_bids.rbegin()->first);
                REQUIRE(book.bestBid()->quantity == model_bids.rbegin()->second);
            }
            if (model_asks.empty()) {
                REQUIRE_FALSE(book.bestAsk().has_value());
            } else {
                REQUIRE(book.bestAsk()->price == model_asks.begin()->first);
                REQUIRE(book.bestAsk()->quantity == model_asks.begin()->second);
            }
        }
    }
    // The book can never end up crossed.
    if (book.bestBid() && book.bestAsk()) REQUIRE(book.bestBid()->price < book.bestAsk()->price);
}
