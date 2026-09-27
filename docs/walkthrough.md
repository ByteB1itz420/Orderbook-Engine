# Walkthrough: the order book engine, from zero

This note explains the domain first, then maps every idea to the code. Read
it alongside the source; interviewers will ask you to connect the two.

## 1. What an exchange actually does

An exchange is a matching service. People send **orders** ("I want to buy 100
shares of XYZ at Rs 100 or less"), and the exchange pairs buyers with
sellers. The data structure at the center of it is the **limit order book**
(LOB): the list of all outstanding buy and sell orders for one symbol,
organized so the exchange can instantly find the best available trade.

Terms:

- **Bid**: a resting buy order. "Resting" means it sits on the book waiting.
- **Ask (or offer)**: a resting sell order.
- **Best bid**: the highest price any buyer is currently willing to pay.
- **Best ask**: the lowest price any seller is currently willing to accept.
- **Spread**: best ask minus best bid. In a healthy book this is positive;
  if a buy and sell ever overlap in price, they should have traded already.
- **Aggressor**: the incoming order that crosses the spread and triggers a
  trade. The resting order is the passive side.

A tiny book:

```
asks (sellers)            bids (buyers)
100.30  x  40             100.05  x  60   <- best bid
100.20  x  80  <- best    100.00  x 125
               ask         99.90  x  75
spread = 100.20 - 100.05 = 0.15
```

## 2. Price-time priority

When an aggressive order arrives, who does it trade with? Exchanges answer
with one rule: **best price first; at the same price, whoever arrived first**.

Worked example. Asks rest at 100.20: order A (80 shares, arrived 9:30:00.1),
then order B (50 shares, arrived 9:30:00.2). Someone sends "buy 100 at
100.25".

1. The buyer crosses the spread (willing to pay 100.25, asks at 100.20).
2. Trades happen at **100.20, the resting price**, not 100.25. The passive
   side's price is the trade price; the aggressor gets price improvement.
3. Order A fills completely (80). Order B, being newer, fills partially (20)
   and keeps 30 shares resting at 100.20.
4. The buy order is done. The book now shows best ask 100.20 x 30.

That is exactly the loop in `OrderBook::match()` (src/book/order_book.cpp).

## 3. Order types

- **Limit**: trade at this price or better, rest if not fully filled.
- **Market**: trade now at whatever prices are available; never rests. It
  "sweeps the book" level by level.
- **IOC (immediate-or-cancel)**: like a limit order, but any unfilled
  remainder is cancelled immediately instead of resting.

Also two bookkeeping operations: **cancel** (remove a resting order) and
**replace/amend** (change price or quantity; exchanges treat this as cancel
+ new order, so the order loses its place in the time queue).

**Self-trade prevention (STP)**: firms run many strategies that can
accidentally trade with themselves, which looks like manipulation ("wash
trading"). Exchanges offer STP modes; we implement *cancel resting*: if the
incoming order would match a resting order from the same owner, the resting
order is cancelled instead. See the `owner` field on `Order` and
`SelfTradeMode` in src/book/order_book.hpp.

## 4. Replay and LOBSTER data

Exchanges broadcast every message that touches the book (adds, cancels,
trades) as a **feed**. If you record the feed, you can **replay** it later
through your own book and rebuild exactly what the exchange's book looked
like. That is how trading firms test strategies on real market history.

LOBSTER is a popular academic dataset of NASDAQ feed recordings. Its message
file is a CSV: `time, type, order_id, size, price, direction`. Our
`LobsterParser` (src/feed/lobster_parser.cpp) turns each line into a typed
`Event` (src/feed/events.hpp), and the `ReplayEngine`
(src/replay/replay_engine.cpp) applies events to the book in order.

One LOBSTER quirk worth knowing: an aggressive incoming order never appears
as an "add". You only see its effect: an execution message against the
resting order. So replaying LOBSTER exercises the book's bookkeeping (adds,
cancels, reductions), while our own matching loop is validated by the
synthetic test suite in tests/test_matching.cpp.

**Determinism**: replaying the same file must produce a byte-identical
result, every time. No wall clocks, no hash-map iteration order, no doubles.
That is why timestamps are parsed by hand into integer nanoseconds and
prices are integer ticks throughout. The determinism test lives in
tests/test_lobster_replay.cpp.

## 5. Why latency matters

An HFT firm's edge decays in microseconds: if your system reacts to a price
change 50 us slower than a competitor, the trade is gone. That is why the
requirements set per-event targets (p50 / p99 / p99.9) and why the code is
built around avoiding work on the **hot path** (the code that runs per
market event):

- **Integer ticks, never doubles.** Doubles have rounding error and can make
  two runs disagree; integers are exact and fast to compare.
- **No allocation per event.** Orders come from a preallocated pool
  (src/book/order_pool.hpp) instead of `new` per order; memory allocation is
  slow and has unpredictable latency spikes.
- **The right asymptotics.** Best bid/ask must be O(1) to read; the book
  keeps prices in sorted order so the best level is always at the edge.

This version uses `std::map` + `std::deque` on purpose: it is the correct,
readable **baseline**. Later milestones swap in cache-friendly structures
and measure the delta, which is the interview story: "here is naive, here is
optimized, here is the measured difference."

## 6. Map: concept -> code

| Concept | Where |
| --- | --- |
| Order, side, order types | src/book/order.hpp |
| Integer tick prices, ids, timestamps | src/book/types.hpp |
| One price level, FIFO queue of orders | src/book/price_level.hpp |
| The book: add/cancel/reduce/replace, best bid/ask | src/book/order_book.hpp/.cpp |
| The matching loop (price-time priority) | `OrderBook::match()` |
| Self-trade prevention | `SelfTradeMode`, `Order::owner` |
| A single fill | src/book/trade.hpp |
| Preallocated order memory | src/book/order_pool.hpp |
| Feed event types | src/feed/events.hpp |
| LOBSTER CSV -> Event | src/feed/lobster_parser.cpp |
| Replay driver | src/replay/replay_engine.cpp |
| Matching behavior tests | tests/test_matching.cpp |
| Book API tests | tests/test_order_book_api.cpp |
| LOBSTER parsing/replay/determinism | tests/test_lobster_replay.cpp |
| Sample data + format cheat-sheet | data/lobster_sample.csv, data/README.md |

## 7. Likely interview questions, with short answers

**Q: Why integer prices instead of doubles?**
Doubles are approximate (0.1 has no exact binary form), so comparisons can
disagree across runs and platforms, which breaks determinism and can cross
orders that should not cross. Integer ticks are exact, fast, and match what
exchanges actually send.

**Q: Why does the trade print at the resting price?**
Price priority rewards the passive side for providing liquidity: the
aggressor crossed into the resting order's price, so that is the fair print.
It also gives the aggressor price improvement when its limit was better.

**Q: Why FIFO within a price level?**
Time priority rewards showing interest early. Without it, everyone would
hide orders until the last moment and the book would carry no information.

**Q: What is the complexity of best bid/ask, add, cancel here?**
Best bid/ask: O(1) (edge of the ordered map). Add: O(log n) levels plus O(1)
amortized queue push. Cancel: O(log n) to find the level via the id index,
then O(k) within the level for the baseline; the optimized engine keeps an
id -> iterator index to make that O(1).

**Q: Why does replace lose time priority?**
Otherwise a trader could keep a top queue spot forever by amending instead
of re-queuing. Exchanges treat a price/qty change as cancel + new order;
some venues keep priority for qty-*decreases*, which is a fine v2 feature.

**Q: How do you guarantee determinism?**
Integer ticks, hand-parsed integer nanosecond timestamps (never via double),
no wall-clock reads in the engine, no iteration over unordered containers,
and a CI test that replays the same file twice and diffs the output.

**Q: Why a pool for orders?**
`new`/`delete` per order costs hundreds of nanoseconds with jitter from the
allocator, which shows up in p99+. A preallocated pool makes acquisition
O(1) and predictable, keeps orders cache-local, and removes allocation from
the hot path entirely once warmed up.

**Q: How is this different from the optimized version you would build?**
Baseline: std::map (red-black tree, pointer-heavy, poor cache locality) and
std::deque. Optimized: flat arrays indexed by tick price, pool-allocated
orders with intrusive links, branch-light matching. The repo keeps the
baseline so the optimization delta is measurable.

**Q: What is self-trade prevention and why does it exist?**
Large firms run many algos that can cross each other; trading with yourself
is a wash trade and can be illegal. STP modes (cancel resting, cancel
incoming, cancel both) prevent it. Ours cancels the resting order when the
owner tags match.

**Q: What would break at real exchange scale?**
Per-order std::deque nodes (cache), the O(k) cancel within a level, and no
support for multiple symbols. Also real feeds arrive over multicast with
gap recovery; this engine assumes a complete, ordered recording.
