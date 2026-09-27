#include <fstream>
#include <sstream>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "feed/parser.hpp"
#include "replay/replay_engine.hpp"

using lob::AddOrderEvent;
using lob::Event;
using lob::ExecutionEvent;
using lob::LobsterParser;
using lob::ReduceOrderEvent;
using lob::ReplayEngine;
using lob::Side;

TEST_CASE("LOBSTER line parsing", "[lobster]") {
    LobsterParser parser;

    // time,type,id,size,price,direction
    const auto add = parser.parseLine("34200.000000001,1,1001,100,1000000,1");
    REQUIRE(add.has_value());
    REQUIRE(std::holds_alternative<AddOrderEvent>(*add));
    const auto& a = std::get<AddOrderEvent>(*add);
    REQUIRE(a.order.id == 1001);
    REQUIRE(a.order.side == Side::Buy);
    REQUIRE(a.order.price == 1000000);  // $100.0000 in integer ticks
    REQUIRE(a.order.quantity == 100);
    // 34200 seconds + 1 nanosecond, parsed without doubles.
    REQUIRE(a.ts == 34200LL * 1'000'000'000LL + 1);

    const auto reduce = parser.parseLine("34200.500000000,2,1001,25,1000000,1");
    REQUIRE(reduce.has_value());
    REQUIRE(std::holds_alternative<ReduceOrderEvent>(*reduce));
    REQUIRE(std::get<ReduceOrderEvent>(*reduce).quantity == 25);

    const auto exec = parser.parseLine("34200.600000000,4,2001,80,1002000,-1");
    REQUIRE(exec.has_value());
    REQUIRE_FALSE(std::get<ExecutionEvent>(*exec).hidden);

    const auto hidden = parser.parseLine("34200.800000000,5,9999,10,1002000,-1");
    REQUIRE(hidden.has_value());
    REQUIRE(std::get<ExecutionEvent>(*hidden).hidden);

    // Trading halts and malformed lines are not book events, and never crash.
    REQUIRE_FALSE(parser.parseLine("34201.0,7,0,0,0,0").has_value());
    REQUIRE_FALSE(parser.parseLine("").has_value());
    REQUIRE_FALSE(parser.parseLine("not,a,csv,line").has_value());
    REQUIRE_FALSE(parser.parseLine("1,1,2,3").has_value());
    REQUIRE_FALSE(parser.parseLine("34200.0,1,1001,-5,1000000,1").has_value());
}

namespace {
// Replay a reader's whole stream and return a canonical book summary, used
// both for the golden master and the determinism check below.
std::string replaySummary(std::istream& input) {
    LobsterParser parser;
    ReplayEngine engine;
    std::string line;
    while (std::getline(input, line)) {
        if (const auto event = parser.parseLine(line)) engine.process(*event);
    }
    std::ostringstream out;
    const auto& book = engine.book();
    out << "orders=" << book.restingOrderCount() << " bid_levels=" << book.bidLevels()
        << " ask_levels=" << book.askLevels();
    if (const auto bid = book.bestBid()) out << " bid=" << bid->price << "x" << bid->quantity;
    if (const auto ask = book.bestAsk()) out << " ask=" << ask->price << "x" << ask->quantity;
    return out.str();
}
}  // namespace

TEST_CASE("replaying the sample produces the expected book (golden master)", "[lobster]") {
    std::ifstream file(std::string(LOB_DATA_DIR) + "/lobster_sample.csv");
    REQUIRE(file.good());

    // Hand-computed from data/lobster_sample.csv:
    //   bids: 1000500x60 (id 1004), 1000000x125 (ids 1001+1002)
    //   asks: 1003000x40 (id 2002); 2001 fully executed, 1003 deleted,
    //   hidden execution had no effect.
    REQUIRE(replaySummary(file) == "orders=4 bid_levels=2 ask_levels=1 bid=1000500x60 ask=1003000x40");
}

TEST_CASE("determinism: two replays of the same file agree byte for byte", "[lobster]") {
    const std::string path = std::string(LOB_DATA_DIR) + "/lobster_sample.csv";
    std::ifstream first(path), second(path);
    REQUIRE(first.good());
    REQUIRE(second.good());
    REQUIRE(replaySummary(first) == replaySummary(second));
}
