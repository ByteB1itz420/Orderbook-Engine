#include "book/order_book.hpp"

#include <limits>
#include <unordered_set>

namespace lob {

bool OrderBook::addOrder(const Order& order) {
    if (order.quantity <= 0 || (order.type != OrderType::Market && order.price <= 0)) return false;
    if (index_.find(order.id) != index_.end()) return false;

    // Every incoming order tries to trade first. Only a Limit remainder rests.
    Order incoming = order;
    match(incoming);
    if (incoming.quantity <= 0) return true;       // fully filled
    if (incoming.type != OrderType::Limit) return true;  // IOC/Market leftover dies

    Levels& levels = sideLevels(*this, incoming.side);
    auto [it, inserted] = levels.try_emplace(incoming.price, incoming.price);
    (void)inserted;
    it->second.pushBack(incoming);
    index_.emplace(incoming.id, std::pair{incoming.side, incoming.price});
    return true;
}

bool OrderBook::addObservedOrder(const Order& order) {
    if (order.type != OrderType::Limit || order.price <= 0 || order.quantity <= 0 ||
        index_.contains(order.id)) return false;
    Levels& levels = sideLevels(*this, order.side);
    auto [it, inserted] = levels.try_emplace(order.price, order.price);
    (void)inserted;
    it->second.pushBack(order);
    index_.emplace(order.id, std::pair{order.side, order.price});
    return true;
}

bool OrderBook::cancelOrder(OrderId id) {
    const auto found = index_.find(id);
    if (found == index_.end()) return false;
    auto [side, price] = found->second;
    Levels& levels = sideLevels(*this, side);
    auto level = levels.find(price);
    if (level == levels.end()) return false;
    const bool removed = level->second.remove(id);
    if (level->second.empty()) levels.erase(level);
    if (removed) index_.erase(found);
    return removed;
}

bool OrderBook::reduceOrder(OrderId id, Quantity delta) {
    if (delta <= 0) return false;
    const auto found = index_.find(id);
    if (found == index_.end()) return false;
    auto [side, price] = found->second;
    Levels& levels = sideLevels(*this, side);
    auto level = levels.find(price);
    if (level == levels.end()) return false;
    bool removed = false;
    if (!level->second.reduce(id, delta, removed)) return false;
    if (level->second.empty()) levels.erase(level);  // no touch of `level` below
    if (removed) index_.erase(found);
    return true;
}

bool OrderBook::replaceOrder(OrderId id, Price new_price, Quantity new_quantity) {
    const auto found = index_.find(id);
    if (found == index_.end() || new_price <= 0 || new_quantity <= 0) return false;
    const Side side = found->second.first;
    if (!cancelOrder(id)) return false;
    Order replacement{};
    replacement.id = id;
    replacement.side = side;
    replacement.type = OrderType::Limit;
    replacement.price = new_price;
    replacement.quantity = new_quantity;
    // Loses time priority on re-add, matching common exchange amend rules.
    return addOrder(replacement);
}

std::optional<TopOfBook> OrderBook::bestBid() const {
    if (bids_.empty()) return std::nullopt;
    const auto& [price, level] = *bids_.rbegin();
    return TopOfBook{price, level.totalQuantity()};
}

std::optional<TopOfBook> OrderBook::bestAsk() const {
    if (asks_.empty()) return std::nullopt;
    const auto& [price, level] = *asks_.begin();
    return TopOfBook{price, level.totalQuantity()};
}

std::vector<OrderBook::LevelView> OrderBook::depth(Side side, std::size_t limit) const {
    std::vector<LevelView> result;
    const Levels& levels = (side == Side::Buy) ? bids_ : asks_;
    result.reserve(std::min(limit, levels.size()));
    if (side == Side::Buy) {
        for (auto it = levels.rbegin(); it != levels.rend() && result.size() < limit; ++it)
            result.push_back({it->first, it->second.totalQuantity(), it->second.orderCount()});
    } else {
        for (auto it = levels.begin(); it != levels.end() && result.size() < limit; ++it)
            result.push_back({it->first, it->second.totalQuantity(), it->second.orderCount()});
    }
    return result;
}

bool OrderBook::checkInvariants() const {
    std::size_t count = 0;
    for (const Side side : {Side::Buy, Side::Sell}) {
        const Levels& levels = (side == Side::Buy) ? bids_ : asks_;
        for (const auto& [price, level] : levels) {
            if (price <= 0 || level.empty() || level.price() != price) return false;
            if (level.totalQuantity() <= 0) return false;
            count += level.orderCount();
        }
    }
    if (count != index_.size()) return false;
    // A historical feed may temporarily present crossed visible quotes. A matching
    // simulator may not: this check is made separately in its tests.
    return true;
}

void OrderBook::match(Order& incoming) {
    Levels& opposite_levels = (incoming.side == Side::Buy) ? asks_ : bids_;

    while (incoming.quantity > 0 && !opposite_levels.empty()) {
        // Best price on the opposite side: lowest ask for a buyer, highest
        // bid for a seller.
        auto best = (incoming.side == Side::Buy) ? opposite_levels.begin()
                                                 : std::prev(opposite_levels.end());

        // Limit orders only trade at their price or better. Market orders
        // take whatever is there; an empty opposite side ends the loop.
        if (incoming.type == OrderType::Limit) {
            const bool crosses = (incoming.side == Side::Buy)
                                     ? best->first <= incoming.price
                                     : best->first >= incoming.price;
            if (!crosses) break;
        }

        PriceLevel& level = best->second;
        const Order& resting = level.front();  // time priority within the level

        // Self-trade prevention: same owner on both sides. CancelResting
        // kills the resting order and the incoming order keeps sweeping.
        if (stp_ == SelfTradeMode::CancelResting && incoming.owner != 0 &&
            resting.owner == incoming.owner) {
            const OrderId id = resting.id;
            level.fillFront(resting.quantity);  // pops the resting order
            index_.erase(id);
            if (level.empty()) opposite_levels.erase(best);
            continue;
        }

        const Quantity traded = std::min(incoming.quantity, resting.quantity);
        if (on_trade_) {
            Trade trade{};
            trade.ts = incoming.timestamp;
            trade.price = resting.price;  // trades print at the resting price
            trade.quantity = traded;
            trade.aggressor = incoming.side;
            trade.buyOrderId = (incoming.side == Side::Buy) ? incoming.id : resting.id;
            trade.sellOrderId = (incoming.side == Side::Sell) ? incoming.id : resting.id;
            on_trade_(trade);
        }

        const OrderId resting_id = resting.id;
        const bool resting_filled = (traded == resting.quantity);
        level.fillFront(traded);
        if (resting_filled) index_.erase(resting_id);
        incoming.quantity -= traded;
        if (level.empty()) opposite_levels.erase(best);
    }
}

}  // namespace lob
