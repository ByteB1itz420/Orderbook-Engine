# orderbook-engine

A C++20 limit order book and exchange event replay engine, built to be
measured: strict price-time matching, deterministic replay of real market
data, and honest p50 / p99 / p99.9 latency numbers.

Status: **milestone 1 complete** - matching engine (strict price-time
priority, limit/market/IOC, self-trade prevention), LOBSTER parser, replay
engine with determinism guarantees, and CI with sanitizers. New to order
books? Start with docs/walkthrough.md.

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

## Next up

Milestone 2 (per the requirements doc): the replay CLI (`replay --input ...
--log ... --bench`), then measurement and optimization against this baseline.

## Rules of the road

- Prices are integer ticks, never doubles.
- No allocation on the hot path after startup (orders come from the pool).
- Determinism is a contract: same input + same config = byte-identical log.
- Every benchmark is one command and the README reproduces headline numbers.
