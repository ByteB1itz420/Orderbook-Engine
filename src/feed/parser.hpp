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

// Parser for the LOBSTER message CSV.
//
// One line per message: time, type, order_id, size, price, direction
//   time:      seconds after midnight, decimal (e.g. 34200.000000001)
//   type:      1 new limit order, 2 partial cancel, 3 full delete,
//              4 visible execution, 5 hidden execution, 7 trading halt
//   order_id:  exchange-assigned id
//   size:      shares
//   price:     dollars * 10000 (already integer ticks)
//   direction: 1 buy, -1 sell (for type 4/5: side of the RESTING order)
//
// Note the LOBSTER quirk: a marketable incoming order never appears as a
// type 1. You only see its effect as a type 4 against the resting order.
class LobsterParser : public IParser {
  public:
    std::optional<Event> parseLine(std::string_view line) override;
};

}  // namespace lob
