#include "book/compact_book.hpp"

#include <algorithm>
#include <limits>

namespace lob {

std::size_t CompactBook::hash(OrderId id) {
    id ^= id >> 33; id *= 0xff51afd7ed558ccdULL;
    id ^= id >> 33; id *= 0xc4ceb9fe1a85ec53ULL;
    return static_cast<std::size_t>(id ^ (id >> 33));
}

CompactBook::CompactBook(std::size_t max_orders, std::size_t max_levels)
    : nodes_(max_orders), levels_(max_levels),
      slots_(std::max<std::size_t>(8, max_orders * 4)) {
    for (std::size_t i = 0; i < nodes_.size(); ++i)
        nodes_[i].free_next = (i + 1 < nodes_.size()) ? static_cast<int>(i + 1) : -1;
    for (std::size_t i = 0; i < levels_.size(); ++i)
        levels_[i].free_next = (i + 1 < levels_.size()) ? static_cast<int>(i + 1) : -1;
    free_node_ = nodes_.empty() ? -1 : 0;
    free_level_ = levels_.empty() ? -1 : 0;
}

int CompactBook::lookup(OrderId id) const {
    const auto n = slots_.size();
    for (std::size_t j = 0, h = hash(id) % n; j < n; ++j, h = (h + 1) % n) {
        const auto& slot = slots_[h];
        if (slot.state == Slot::Empty) return -1;
        if (slot.state == Slot::Live && slot.id == id) return slot.node;
    }
    return -1;
}

int CompactBook::slotForInsert(OrderId id) const {
    const auto n = slots_.size(); int deleted = -1;
    for (std::size_t j = 0, h = hash(id) % n; j < n; ++j, h = (h + 1) % n) {
        const auto& slot = slots_[h];
        if (slot.state == Slot::Live && slot.id == id) return -1;
        if (slot.state == Slot::Deleted && deleted < 0) deleted = static_cast<int>(h);
        if (slot.state == Slot::Empty) return deleted >= 0 ? deleted : static_cast<int>(h);
    }
    return deleted;
}

void CompactBook::eraseId(OrderId id) {
    const auto n = slots_.size();
    for (std::size_t j = 0, h = hash(id) % n; j < n; ++j, h = (h + 1) % n) {
        auto& slot = slots_[h];
        if (slot.state == Slot::Empty) return;
        if (slot.state == Slot::Live && slot.id == id) { slot.state = Slot::Deleted; return; }
    }
}

int CompactBook::findLevel(Side side, Price price) const {
    for (int p = heads_[sideIndex(side)]; p != -1; p = levels_[p].next)
        if (levels_[p].price == price) return p;
    return -1;
}

int CompactBook::makeLevel(Side side, Price price) {
    if (free_level_ == -1) return -1;
    int prior = -1; int p = heads_[sideIndex(side)];
    while (p != -1 && (side == Side::Buy ? levels_[p].price > price : levels_[p].price < price)) {
        prior = p; p = levels_[p].next;
    }
    const int index = free_level_; free_level_ = levels_[index].free_next;
    levels_[index] = Level{};
    auto& level = levels_[index]; level.live = true; level.price = price;
    level.prev = prior; level.next = p;
    if (prior == -1) heads_[sideIndex(side)] = index;
    else levels_[prior].next = index;
    if (p != -1) levels_[p].prev = index;
    return index;
}

void CompactBook::retireLevel(Side side, int index) {
    const auto prev = levels_[index].prev, next = levels_[index].next;
    if (prev == -1) heads_[sideIndex(side)] = next;
    else levels_[prev].next = next;
    if (next != -1) levels_[next].prev = prev;
    levels_[index] = Level{}; levels_[index].free_next = free_level_;
    free_level_ = index;
}

int CompactBook::takeNode() {
    if (free_node_ == -1) return -1;
    const int index = free_node_; free_node_ = nodes_[index].free_next;
    nodes_[index] = Node{}; nodes_[index].live = true;
    return index;
}
void CompactBook::releaseNode(int index) {
    nodes_[index] = Node{}; nodes_[index].free_next = free_node_;
    free_node_ = index; --live_;
}
void CompactBook::append(int level, int node) {
    auto& l = levels_[level]; auto& n = nodes_[node];
    n.level = level; n.prev = l.tail;
    if (l.tail != -1) nodes_[l.tail].next = node;
    else l.head = node;
    l.tail = node; l.quantity += n.order.quantity;
    ++live_;
}
void CompactBook::unlink(int node) {
    auto& n = nodes_[node]; auto& l = levels_[n.level];
    if (n.prev != -1) nodes_[n.prev].next = n.next; else l.head = n.next;
    if (n.next != -1) nodes_[n.next].prev = n.prev; else l.tail = n.prev;
    l.quantity -= n.order.quantity;
}
void CompactBook::fillHead(int level, Quantity qty) {
    const int index = levels_[level].head;
    auto& n = nodes_[index]; n.order.quantity -= qty;
    levels_[level].quantity -= qty;
    if (n.order.quantity == 0) {
        // unlink expects the remaining quantity (now zero).
        unlink(index); eraseId(n.order.id); releaseNode(index);
    }
}

bool CompactBook::addOrder(const Order& order) {
    if (order.quantity <= 0 || (order.type != OrderType::Market && order.price <= 0) ||
        lookup(order.id) != -1) return false;
    // A crossing order could partially trade then need a level we cannot store.
    // Reject it before any fill rather than producing a half-accepted event.
    if (order.type == OrderType::Limit && (free_node_ == -1 ||
        (findLevel(order.side, order.price) == -1 && free_level_ == -1))) return false;
    Order incoming = order;
    match(incoming);
    if (incoming.quantity <= 0 || incoming.type != OrderType::Limit) return true;
    const int slot = slotForInsert(incoming.id);
    if (slot < 0 || free_node_ == -1) return false; // bounds verified above
    int level = findLevel(incoming.side, incoming.price);
    if (level == -1) level = makeLevel(incoming.side, incoming.price);
    if (level == -1) return false;
    const int node = takeNode(); nodes_[node].order = incoming;
    append(level, node);
    slots_[slot] = Slot{incoming.id,node,Slot::Live};
    return true;
}

bool CompactBook::cancelOrder(OrderId id) {
    const int node = lookup(id); if (node == -1) return false;
    const Side side = nodes_[node].order.side;
    const int level = nodes_[node].level;
    unlink(node); eraseId(id); releaseNode(node);
    if (levels_[level].head == -1) retireLevel(side,level);
    return true;
}

bool CompactBook::reduceOrder(OrderId id, Quantity amount) {
    if (amount <= 0) return false;
    const int node = lookup(id); if (node == -1) return false;
    if (amount >= nodes_[node].order.quantity) return cancelOrder(id);
    nodes_[node].order.quantity -= amount;
    levels_[nodes_[node].level].quantity -= amount;
    return true;
}

bool CompactBook::replaceOrder(OrderId id, Price price, Quantity quantity) {
    const int node = lookup(id);
    if (node == -1 || price <= 0 || quantity <= 0) return false;
    // Reject a replacement that would need a new level when none is free.
    // The old level may be retired by cancel, making one slot available.
    if (findLevel(nodes_[node].order.side, price) == -1 && free_level_ == -1 &&
        levels_[nodes_[node].level].head != node) return false;
    const Order old = nodes_[node].order;
    if (!cancelOrder(id)) return false;
    Order replacement = old; replacement.price = price;
    replacement.quantity = quantity; replacement.type = OrderType::Limit;
    return addOrder(replacement);
}

void CompactBook::match(Order& incoming) {
    const int side = sideIndex(opposite(incoming.side));
    while (incoming.quantity > 0 && heads_[side] != -1) {
        const int level = heads_[side];
        if (incoming.type != OrderType::Market &&
            (incoming.side == Side::Buy ? levels_[level].price > incoming.price :
                                          levels_[level].price < incoming.price)) break;
        const int node = levels_[level].head;
        const auto& resting = nodes_[node].order;
        const auto id = resting.id;
        const Quantity qty = std::min(incoming.quantity, resting.quantity);
        if (on_trade_) {
            Trade t{}; t.ts = incoming.timestamp; t.price = resting.price;
            t.quantity = qty; t.aggressor = incoming.side;
            t.buyOrderId = incoming.side == Side::Buy ? incoming.id : id;
            t.sellOrderId = incoming.side == Side::Sell ? incoming.id : id;
            on_trade_(t);
        }
        fillHead(level, qty);
        incoming.quantity -= qty;
        if (levels_[level].head == -1) retireLevel(opposite(incoming.side), level);
    }
}

bool CompactBook::top(Side side, Top& out) const {
    const int p = heads_[sideIndex(side)]; if (p == -1) return false;
    out = {levels_[p].price, levels_[p].quantity}; return true;
}

bool CompactBook::checkInvariants() const {
    std::size_t count = 0;
    for (int side = 0; side < 2; ++side) {
        int prev = -1; Price last = 0;
        for (int p = heads_[side]; p != -1; p = levels_[p].next) {
            const auto& level = levels_[p];
            if (!level.live || level.prev != prev || level.price <= 0 || level.head == -1 ||
                (prev != -1 && (side == 0 ? level.price >= last : level.price <= last))) return false;
            Quantity sum = 0; int nprev = -1; std::size_t guard = 0;
            for (int n = level.head; n != -1; n = nodes_[n].next) {
                const auto& node = nodes_[n];
                if (++guard > nodes_.size() || !node.live || node.level != p || node.prev != nprev ||
                    node.order.quantity <= 0 || node.order.price != level.price ||
                    sideIndex(node.order.side) != side || lookup(node.order.id) != n) return false;
                if (sum > std::numeric_limits<Quantity>::max() - node.order.quantity) return false;
                sum += node.order.quantity; nprev = n; ++count;
            }
            if (sum != level.quantity || nprev != level.tail) return false;
            last = level.price; prev = p;
        }
    }
    return count == live_;
}
} // namespace lob
