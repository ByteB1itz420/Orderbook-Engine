#include "feed/parser.hpp"

#include <array>
#include <charconv>
#include <cstdint>

namespace lob {
namespace {

// Split `line` on commas into at most N fields. Returns the field count.
template <std::size_t N>
std::size_t splitCsv(std::string_view line, std::array<std::string_view, N>& out) {
    std::size_t count = 0;
    std::size_t start = 0;
    while (count < N) {
        const std::size_t comma = line.find(',', start);
        if (comma == std::string_view::npos) {
            out[count++] = line.substr(start);
            break;
        }
        out[count++] = line.substr(start, comma - start);
        start = comma + 1;
    }
    return count;
}

bool parseInt(std::string_view text, std::int64_t& value) {
    if (text.empty()) return false;
    const auto* first = text.data();
    const auto* last = text.data() + text.size();
    const auto result = std::from_chars(first, last, value);
    return result.ec == std::errc{} && result.ptr == last;
}

// "34200.000000001" -> nanoseconds after midnight. Parsed by hand (never
// through double) so the replay stays bit-exact deterministic.
bool parseTimeNs(std::string_view text, TimestampNs& out) {
    const std::size_t dot = text.find('.');
    const std::string_view seconds_part = text.substr(0, dot);
    std::int64_t seconds = 0;
    if (!parseInt(seconds_part, seconds) || seconds < 0) return false;

    std::int64_t nanos = 0;
    if (dot != std::string_view::npos) {
        std::string_view frac = text.substr(dot + 1);
        if (frac.empty() || frac.size() > 9) return false;
        for (const char c : frac) {
            if (c < '0' || c > '9') return false;
            nanos = nanos * 10 + (c - '0');
        }
        for (std::size_t i = frac.size(); i < 9; ++i) nanos *= 10;
    }
    out = seconds * 1'000'000'000LL + nanos;
    return true;
}

}  // namespace

std::optional<Event> LobsterParser::parseLine(std::string_view line) {
    // Trim a trailing CR so Windows-style files parse the same.
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    if (line.empty()) return std::nullopt;

    std::array<std::string_view, 6> fields{};
    if (splitCsv(line, fields) != 6) return std::nullopt;

    TimestampNs ts = 0;
    std::int64_t type = 0, id = 0, size = 0, price = 0, direction = 0;
    if (!parseTimeNs(fields[0], ts)) return std::nullopt;
    if (!parseInt(fields[1], type)) return std::nullopt;
    if (!parseInt(fields[2], id) || id < 0) return std::nullopt;
    if (!parseInt(fields[3], size) || size <= 0) return std::nullopt;
    if (!parseInt(fields[4], price) || price <= 0) return std::nullopt;
    if (!parseInt(fields[5], direction)) return std::nullopt;

    switch (type) {
        case 1: {  // new limit order
            if (direction != 1 && direction != -1) return std::nullopt;
            AddOrderEvent event{};
            event.ts = ts;
            event.order.id = static_cast<OrderId>(id);
            event.order.side = (direction == 1) ? Side::Buy : Side::Sell;
            event.order.type = OrderType::Limit;
            event.order.price = price;
            event.order.quantity = size;
            event.order.timestamp = ts;
            return Event{event};
        }
        case 2:  // partial cancel: reduce by `size`
            return Event{ReduceOrderEvent{ts, static_cast<OrderId>(id), size}};
        case 3:  // full delete
            return Event{CancelOrderEvent{ts, static_cast<OrderId>(id)}};
        case 4:  // visible execution against a resting order
            return Event{ExecutionEvent{ts, static_cast<OrderId>(id), price, size, false}};
        case 5:  // hidden execution: no effect on the visible book
            return Event{ExecutionEvent{ts, static_cast<OrderId>(id), price, size, true}};
        default:
            // 7 (trading halt) and anything unknown: not book events.
            return std::nullopt;
    }
}

}  // namespace lob
