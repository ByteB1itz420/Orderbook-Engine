# Matching and feed semantics

One C++20 book represents one symbol in one thread. Prices and sizes are signed 64-bit integers; a valid resting limit order has positive price and quantity. In LOBSTER files, one dollar equals 10,000 price units. Order IDs are unsigned and unique among *resting* orders. A filled or canceled ID can later be reused by the simulator; the historical feed supplies its own IDs.

## Incoming commands (simulation)

Highest bid and lowest ask have price priority. Within one price, the oldest resting order fills first. An incoming buy crosses an ask at or below its limit; a sell crosses a bid at or above its limit. A trade prints at the resting price. Partial fills reduce both sides. Market and IOC remainders expire; limit remainders rest. `replaceOrder` currently cancels and re-adds **every** amendment, losing queue priority, even same-price size reductions. A priority-preserving size reduction is planned but not yet implemented. Invalid replacement leaves the original order untouched. `SelfTradeMode::CancelResting` cancels a same-owner resting order and continues the sweep; owner 0 means anonymous. These are simulation choices, not Nasdaq rule parity.

## Observed historical updates

LOBSTER message type 1 is an observed resting add, not a new command to match. Types 2 and 3 reduce/cancel that ID; type 4 reduces it by an observed visible execution; type 5 leaves visible depth unchanged. The aggressive order that caused type 4 is not present in the feed. Matching simulation and historical visible-book reconstruction are separate paths. The historical sample starts after an unknown pre-window state and only reports updates within selected displayed depth, so unknown IDs and full-depth mismatches are expected. A paired same-price size-delta check is a narrower validation, not proof of complete reconstruction.

## Data structures

M1 deliberately uses `std::map` price levels, `std::deque` FIFO within each level, and `std::unordered_map` ID to `(side, price)`. The map finds best price at its edge and keeps sparse prices sorted; deque preserves FIFO; ID lookup gets the level, but cancel still scans orders *within* that price. This is a readable correct baseline, not an allocation-free implementation. The separate `OrderPool` is not wired into this book. A pool-backed intrusive FIFO plus stable ID-to-node lookup is a candidate optimization only after independent tests and measured comparison. No zero-allocation or fixed tail-latency claim applies to M1.
