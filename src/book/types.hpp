#pragma once

#include <cstdint>

namespace lob {

// All prices are integer ticks, never doubles. A price of 123.45 with a tick
// size of 0.01 is stored as 12345 ticks. Doubles on the hot path invite
// rounding bugs and non-determinism.
using Price = std::int64_t;
using Quantity = std::int64_t;
using OrderId = std::uint64_t;
using TimestampNs = std::int64_t;  // nanoseconds since session start / exchange epoch

}  // namespace lob
