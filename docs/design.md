# Design notes

## v1 architecture
```
feed file -> parser -> Event variant -> ReplayEngine -> OrderBook
                                          |
                                          +-> trade/audit log (deterministic)
```

## Deliberate choices
- **Integer ticks for prices.** Never doubles on the hot path: rounding bugs
  and non-determinism. LOBSTER prices are already integer (dollars x 10000).
- **Event-sourced engine.** Every state change comes from an Event, so a
  research process can later consume the same stream (the QR layer), and
  "book equals rebuild from event log" is a natural invariant test.
- **Naive baseline first.** std::map + std::deque, correct before fast. The
  optimized engine (pool-backed, cache-friendly) is benchmarked against this
  baseline in milestone 6-7; the delta is the interview story.
- **Determinism as a contract.** No wall-clock, no hash-order iteration.
  Two replays of the same file must produce byte-identical logs; CI diffs them.

## Current state
- `OrderBook::match()` and `LobsterParser` were implemented in the M1 bundle.
- The CLI now runs synthetic matching and observed historical replay.
- The C++ book is still the standard-container baseline. `OrderPool` is not connected to it.
- See STATUS.md for verified and pending work; do not infer status from old milestones.
