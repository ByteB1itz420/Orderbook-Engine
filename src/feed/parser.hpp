#pragma once

#include <optional>
#include <string_view>

#include "feed/events.hpp"

namespace lob {

// One parsed line in, one event out. Parsers are pure functions over a text
// record: no I/O, no state beyond the line, so they are trivially fuzzable
// (throw random bytes at them; bad input must return nullopt, never crash).
class IParser {
  public:
    virtual ~IParser() = default;
    virtual std::optional<Event> parseLine(std::string_view line) = 0;
};

// TODO(ayush, milestone 1 cont.): LobsterParser : IParser.
// Input format: LOBSTER message CSV
//   time, type, order_id, size, price, direction
// Map message types: 1 -> AddOrderEvent, 3 -> CancelOrderEvent (full delete),
// 2/4/5 -> ExecutionEvent variants (see the LOBSTER readme in data/),
// direction +1 buy / -1 sell, price in 1/10000 dollars (already ticks x100).
// Start with the free samples: https://lobsterdata.com/info/DataSamples.php

}  // namespace lob
