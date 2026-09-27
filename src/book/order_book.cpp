#include "book/order_book.hpp"

namespace lob {

bool OrderBook::addOrder(const Order& order) {
    if (order.quantity <= 0) return false;
    if (index_.find(order.id) != index_.end()) return false;

    // Any incoming order may trade first. For milestone 1, match() is a
    // marked TODO and does nothing, so every limit order simply rests.
    Order incoming = order;
    match(incoming);

    if (incoming.quantity <= 0) return true;  // fully filled by match()
    if (incoming.type != OrderType::Limit) {
        // Ioc/Market leftovers never rest.
        // NOTE: until match() is implemented, limit orders rest below and
        // Ioc/Market are dropped here. Revisit when writing the loop.
        return true;
    }

    Levels& levels = sideLevels(*this, incoming.side);
    auto [it, inserted] =
        levels.try_emplace(incoming.price, incoming.price);
    (void)inserted;
    it->second.pushBack(incoming);
    index_.emplace(incoming.id, std::pair{incoming.side, incoming.price});
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

bool OrderBook::replaceOrder(OrderId id, Price new_price, Quantity new_quantity) {
    const auto found = index_.find(id);
    if (found == index_.end()) return false;
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

void OrderBook::match(Order& incoming) {
    // TODO(ayush): the matching loop. See the long comment in the header.
    // Until this does something, no trades happen and every limit order rests.
    (void)incoming;
}

}  // namespace lob
