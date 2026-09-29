#include <array>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "feed/parser.hpp"
#include "replay/replay_engine.hpp"

namespace {
using namespace lob;

bool parseDepth(const std::string& line, std::array<std::int64_t, 40>& out) {
    std::istringstream in(line);
    std::string field;
    for (auto& value : out) {
        if (!std::getline(in, field, ',')) return false;
        const auto result = std::from_chars(field.data(), field.data() + field.size(), value);
        if (result.ec != std::errc{} || result.ptr != field.data() + field.size()) return false;
    }
    return !std::getline(in, field, ',');
}

// Compare price and aggregate size for every occupied displayed level. The
// publisher's zero-volume dummy levels are not real orders.
bool sameDisplayedDepth(const OrderBook& book, const std::array<std::int64_t, 40>& snapshot) {
    const auto asks = book.depth(Side::Sell, 10);
    const auto bids = book.depth(Side::Buy, 10);
    for (std::size_t i = 0; i < 10; ++i) {
        for (int side = 0; side < 2; ++side) {
            const auto& actual = side == 0 ? asks : bids;
            const std::size_t col = 4 * i + (side == 0 ? 0 : 2);
            const auto qty = snapshot[col + 1];
            if (qty == 0) { if (actual.size() > i) return false; }
            else if (actual.size() <= i || actual[i].price != snapshot[col] ||
                     actual[i].quantity != qty) return false;
        }
    }
    return true;
}

int replay(const std::string& path, const std::string& depth_path) {
    std::ifstream messages(path), snapshots;
    if (!messages) { std::cerr << "Cannot open message file\n"; return 2; }
    if (!depth_path.empty()) {
        snapshots.open(depth_path);
        if (!snapshots) { std::cerr << "Cannot open paired orderbook file\n"; return 2; }
    }
    LobsterParser parser;
    ReplayEngine engine;
    std::string line, snapshot;
    std::uint64_t rows = 0, applied = 0, unsupported = 0, matched = 0, mismatched = 0;
    std::array<std::int64_t, 40> depth{};
    while (std::getline(messages, line)) {
        ++rows;
        if (snapshots.is_open() && (!std::getline(snapshots, snapshot) ||
                                     !parseDepth(snapshot, depth))) {
            std::cerr << "Invalid/missing paired row " << rows << '\n'; return 2;
        }
        const auto event = parser.parseLine(line);
        if (event) { engine.process(*event); ++applied; }
        else ++unsupported; // includes halts, cross prints, and malformed rows
        if (snapshots.is_open()) {
            if (sameDisplayedDepth(engine.book(), depth)) ++matched;
            else ++mismatched;
        }
    }
    if (snapshots.is_open() && std::getline(snapshots, snapshot)) {
        std::cerr << "Extra paired orderbook rows\n"; return 2;
    }
    const auto& book = engine.book();
    std::cout << "mode=historical-visible-book rows=" << rows << " applied=" << applied
              << " unsupported_or_malformed=" << unsupported;
    if (snapshots.is_open())
        std::cout << " paired_depth_exact=" << matched << " paired_depth_mismatch=" << mismatched;
    std::cout << " resting_orders=" << book.restingOrderCount() << '\n';
    std::cout << "Note: initial state is unavailable; unknown order IDs may occur. "
                 "No exchange-fill or complete-reconstruction claim.\n";
    return 0;
}

int synthetic() {
    OrderBook book;
    std::uint64_t fills = 0;
    book.setTradeHandler([&](const Trade& t) {
        ++fills;
        std::cout << "trade price=" << t.price << " size=" << t.quantity
                  << " buy=" << t.buyOrderId << " sell=" << t.sellOrderId << '\n';
    });
    for (int i = 0; i < 5; ++i) {
        Order ask{}; ask.id = 100 + i; ask.side = Side::Sell;
        ask.price = 10000 + 10 * i; ask.quantity = 20 + i * 5;
        book.addOrder(ask);
    }
    Order buy{}; buy.id = 200; buy.side = Side::Buy; buy.type = OrderType::Limit;
    buy.price = 10020; buy.quantity = 55;
    book.addOrder(buy);
    std::cout << "mode=synthetic fills=" << fills << " resting_orders="
              << book.restingOrderCount() << '\n';
    return book.checkInvariants() ? 0 : 1;
}
} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--synthetic") return synthetic();
    if ((argc == 3 || argc == 5) && std::string(argv[1]) == "--messages" &&
        (argc == 3 || std::string(argv[3]) == "--orderbook"))
        return replay(argv[2], argc == 5 ? argv[4] : "");
    std::cerr << "Usage: replay --synthetic | --messages MESSAGE.csv "
                 "[--orderbook MATCHED_10_LEVELS.csv]\n";
    return 2;
}
