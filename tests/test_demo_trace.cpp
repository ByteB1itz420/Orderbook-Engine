#include <fstream>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "book/order_book.hpp"
using namespace lob;
TEST_CASE("demo trace generated fills match C++ simulator", "[demo]") {
    OrderBook book;
    std::vector<Trade> trades;
    book.setTradeHandler([&](const Trade& t){trades.push_back(t);});
    struct Input { Side side; Price price; Quantity qty; OrderId id; bool cancel=false; };
    const Input events[]{
        {Side::Sell,10030,28,101},{Side::Buy,9980,36,102},
        {Side::Sell,10020,20,103},{Side::Buy,9990,22,104},
        {Side::Sell,10020,18,105},{Side::Buy,10000,26,106},
        {Side::Sell,10040,42,107},{Side::Buy,10020,27,108},
        {Side::Sell,10030,28,101,true},{Side::Buy,10050,55,109},
        {Side::Sell,10010,15,110},{Side::Buy,10020,10,111},
        {Side::Buy,9990,22,104,true},{Side::Sell,10030,24,112},
        {Side::Buy,10040,30,113}
    };
    for(const auto& x:events) {
        if(x.cancel) REQUIRE(book.cancelOrder(x.id));
        else {Order o{};o.id=x.id;o.side=x.side;o.price=x.price;o.quantity=x.qty;
              REQUIRE(book.addOrder(o));}
    }
    REQUIRE(trades.size()==8);
    const Price prices[]{10020,10020,10020,10040,10050,10010,10010,10030};
    const Quantity sizes[]{20,7,11,42,2,10,3,24};
    for(std::size_t i=0;i<trades.size();++i){REQUIRE(trades[i].price==prices[i]);REQUIRE(trades[i].quantity==sizes[i]);}
    REQUIRE(book.bestBid()->price==10040);
    REQUIRE(book.bestBid()->quantity==3);
    REQUIRE_FALSE(book.bestAsk().has_value());
}
