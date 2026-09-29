# Orderbook-Engine: measured report

This project has two distinct tasks. The **matching simulator** takes incoming orders and generates fills under our own price-time contract. The **historical replay** consumes already observed LOBSTER messages to study visible depth; it does not recreate Nasdaq's matching decisions. Nothing here connects to an exchange or establishes a profitable strategy. See [STATUS.md](STATUS.md) for remaining work.

## Correctness and source data

The M1 matching baseline uses `std::map` sorted price levels, `std::deque` FIFO at each level, and an ID index to find the level. It is deliberately easy to read, but cancel scans a level, standard containers allocate, and best-depth aggregation scans that level. The `CompactBook` alternative preallocates fixed arrays for orders and levels, links orders in FIFO queues, and uses a flat ID table. It can cancel a known order in expected O(1), but it scans levels linearly to insert a new price. It is not a full drop-in exchange-equivalent replacement: its capacity is bounded and it does not have the baseline's configurable self-trade prevention. Both implementations passed a seeded 5,000-event comparison for top quotes, resting counts, and generated trades within that shared scope. Release and ASan/UBSan test runs passed 30/30 on the agent host; GitHub CI and Ayush's laptop must be checked separately.

The [publisher's AMZN 2012-06-21 10-level sample](https://php.lobsterdata.com/info/DataSamples.php) zip has SHA-256 `5cff62a609b27aef82285382ad646c4c2facc0a692a60bedda633de64c4aa54f`. Its message and depth files each contain 269,748 rows. The archive is downloaded locally, not redistributed here. A naïve empty-start replay had only **148 exact full-depth rows out of 269,748**. That is expected: the sample starts with nonempty resting state and filters event messages to displayed depth; historical IDs/deeper liquidity are missing. It would be wrong to count the remaining 269,600 as parser errors or to claim a full book reconstruction.

The reproducible [narrower checker](src/replay/validate_depth.py) compares event-price visible-size changes only when that price appears in consecutive snapshots. It found **162,283 matches of 162,283 eligible add/cancel/delete/visible-execution transitions**, plus **2,444 of 2,444 hidden executions** with unchanged snapshots. **105,020 transitions were excluded** for boundaries or other event types; the first row has no preceding snapshot. This confirms a limited message/snapshot consistency relation, not that our C++ simulator regenerated exchange fills or reconstructed the complete book. It is good evidence with an honest denominator, not a shortcut past missing initial state.

## Local timing experiment

The protocol was frozen in commit `f128997` before this result. Raw output is [results/agent_host_benchmark.csv](results/agent_host_benchmark.csv). Machine: containerized Linux x86_64, Intel Xeon Processor @ 2.60GHz, 2 visible logical CPUs sharing one core, 1.9 GiB visible RAM, GCC 11.4.0, CMake Release build (verified effective CMake flags `-O3 -DNDEBUG`). Power governor/turbo and exclusive core pinning were **not controlled**. These numbers belong to the **agent host**, not Ayush's laptop or any production environment.

Workload: each of five repetitions warms the book with 2,000 resting orders, then measures 10,000 calls per operation (add, cancel, modify, one-level market match) with `std::chrono::steady_clock` around each call. Trace construction/output lie outside the timed region. `events_per_s_call_only` is count divided by summed operation-call duration, **not replay throughput**. Percentiles use nearest-rank; p99.9 is the 9,990th of 10,000 samples. Individual per-event clock overhead, shared-host jitter, allocator effects, and highly stylized traffic all limit extrapolation.

| Operation | Baseline median p99, ns (5 runs) | Compact median p99, ns (5 runs) | Reading |
| --- | ---: | ---: | --- |
| Add | 135 | 273 | Compact slower; linear level search costs more here. |
| Cancel | 131 | 62 | Compact faster; ID-to-node unlink helps. |
| Modify | 268 | 221 | Compact modestly faster in this mix. |
| Match | 93 | 177 | Compact slower in this mix. |

The p99 ranges across five runs are in the CSV; the median p99 is *not* pooled p99 across runs. No latency advantage is claimed for the compact book overall. The baseline is both clearer and faster on add/match in this experiment. A stronger timing claim requires a laptop rerun, independent allocation instrumentation, profiling, and a more representative workload, with any changes followed by the same tests. Network, order gateway, feed parsing and dissemination are outside these call timings.

## Reproduction

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
ctest --test-dir build --output-on-failure
./build/replay --synthetic
./build/lob_bench > your_local_benchmark.csv
# Download the publisher's paired AMZN 10-level sample, unzip locally:
python3 src/replay/validate_depth.py --messages /path/to/AMZN_2012-06-21_34200000_57600000_message_10.csv --orderbook /path/to/AMZN_2012-06-21_34200000_57600000_orderbook_10.csv
```

Read [docs/walkthrough.md](docs/walkthrough.md) to explain the design in ordinary words before putting a performance claim on a resume. The report intentionally leaves open any milestone without evidence.
