#include <catch2/catch_test_macros.hpp>

#include "feed/events.hpp"

using lob::AddOrderEvent;
using lob::CancelOrderEvent;
using lob::Event;

TEST_CASE("events round-trip through the variant", "[events]") {
    AddOrderEvent add{};
    add.ts = 42;
    add.order.id = 7;
    add.order.quantity = 100;

    Event event = add;
    REQUIRE(std::holds_alternative<AddOrderEvent>(event));
    const auto& back = std::get<AddOrderEvent>(event);
    REQUIRE(back.order.id == 7);
    REQUIRE(back.ts == 42);

    event = CancelOrderEvent{.ts = 43, .id = 7};
    REQUIRE(std::holds_alternative<CancelOrderEvent>(event));
}
