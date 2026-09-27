# orderbook-engine

A C++20 limit order book and exchange event replay engine, built to be
measured: strict price-time matching, deterministic replay of real market
data, and honest p50 / p99 / p99.9 latency numbers.

Status: **milestone 1** - repo skeleton, core types and container plumbing,
CI with sanitizers. The matching loop is intentionally unimplemented (marked
TODO) while the engine's owner writes it; see "Write the matcher" below.

## Layout

```
src/book/    order book, price levels, order pool, core types
src/feed/    event types + parser interface (LOBSTER parser next)
src/replay/  replay engine + CLI stub
src/bench/   benchmark harness (milestone 5)
tests/       Catch2 tests, incl. matching-test stubs
data/        sample data (large files gitignored)
docs/        design notes
```

## Build and test

```sh
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Requires CMake 3.22+ and a C++20 compiler (gcc 11+ or clang 14+). Catch2 is
fetched automatically on first configure. Sanitizer build:

```sh
cmake -B build-san -DLOB_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-san -j && ctest --test-dir build-san --output-on-failure
```

## Write the matcher

`src/book/order_book.cpp` has `OrderBook::match()` as a marked TODO with the
full spec in the header. Suggested order:

1. Write the matching tests first (stubs in `tests/test_order_book_api.cpp`).
2. Implement the loop: sweep the opposite side best-to-worst at resting
   prices, fill FIFO within a level, pop filled orders and empty levels.
3. Handle leftovers: Limit rests, Ioc/Market drops.
4. Run the invariant tests: best bid < best ask, rebuild-from-log equals live.

## Rules of the road

- Prices are integer ticks, never doubles.
- No allocation on the hot path after startup (orders come from the pool).
- Determinism is a contract: same input + same config = byte-identical log.
- Every benchmark is one command and the README reproduces headline numbers.
