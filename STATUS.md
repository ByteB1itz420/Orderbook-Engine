# Orderbook-Engine: project status

**As of:** September 29, 2026  
**State:** Execution authorized by Ayush at 12:07 IST on September 29, 2026. He asked for the full PLAN.md build and a deployed demo today, with progress commits, a Brag video, and this repository public at the end. Work has begun with the M1 audit. Remaining tasks below are not done merely because the deadline is today.

**Where the work is:** The [private GitHub repository](https://github.com/ByteB1itz420/Orderbook-Engine) now contains the M1 source plus PLAN.md and STATUS.md at commit `729d64a`. An [M1-complete ZIP in Ayush's BITS Drive](https://drive.google.com/file/d/1WNCGPdEwKJRlnZiwoQChqBPA0v3rVmCg/view?usp=drivesdk&authuser=f20230965%40goa.bits-pilani.ac.in) contains actual C++20 source, CMake configuration, tests, CI configuration, and a beginner walkthrough. The ZIP had two local commits; both were included in the first GitHub push. New work under PLAN.md has begun with an audit and clean build; source changes and a baseline documentation push have since been made.

**Evidence boundary:** This snapshot comes from inspecting the Drive ZIP, checking the live GitHub repo, and building/testing on an agent workspace, not from Ayush's machine. The ZIP has 26 initial `TEST_CASE` declarations, and all 26 passed in a fresh Release build using GCC 11.4 on the agent workspace. The same 26 tests passed in a fresh Debug build with ASan/UBSan; two new tests also pass in Release and ASan/UBSan (28/28). GitHub CI and Ayush’s local environment have not yet been rerun. A README says “milestone 1 complete,” but older design notes still call components missing, and the replay CLI is visibly a stub. This log follows inspected code, not broad milestone labels.

## Component progress

Use **Present in M1**, **Partial**, **Not started**, or **Blocked**. A feature is not complete merely because an interface, comment, test stub, or plan mentions it. Check PLAN.md for requirements and keep both files aligned when scope changes.

### Matching engine and data structures - Partial

- **Present in M1 ZIP:** C++20 `OrderBook` implementation and tests for price-time matching, partial fills, market/limit/IOC behavior, cancel, reduce, replace, and optional self-trade prevention. It uses integer prices, `std::map` price levels, `std::deque` FIFO orders, and `std::unordered_map` ID lookup as a clear baseline. An `OrderPool` class exists separately.
- **Open:** Audit exact semantics and edge cases with Ayush; write the frozen contract; decide modify priority and capacity behavior; implement and compare stable pool-backed intrusive FIFO, flat ID lookup, and any justified cache-aware change. The current baseline's cancellation scans a price-level deque, and the pool is not integrated with the `OrderBook`; no verified zero-allocation hot-path claim.

### Historical replay and synthetic flow - Partial

- **Present in M1 ZIP:** LOBSTER-format line parser and a `ReplayEngine` that applies add, cancel, reduce, replace, and visible execution events. Tests replay a **hand-written 10-line CSV fixture**, not an official historical LOBSTER sample. Hidden executions do not change visible depth.
- **Open:** Obtain/verify the official paired message and orderbook sample, establish starting state and supported-event policy, compare reconstructed depth row by row with counts and mismatches, record hashes/provenance, and separate historical observed updates from synthetic incoming orders. `src/replay/main.cpp` now has synthetic and historical modes, locally built and tested; paired-depth validation remains a narrow delta check, not a full book check. Do not claim real historical validation from the hand-written fixture.

### Correctness - Partial

- **Present in M1 ZIP:** 26 declared Catch2 test cases, including matching, parser, hand-written replay, determinism, and randomized reference-model coverage; CMake test setup and CI workflow with GCC/Clang plus ASan/UBSan configuration.
- **Open:** Run on Ayush's environment and check CI after a push. Add explicit per-event integrity/conservation invariants, failure-seed shrinking and saved golden traces, broader boundary cases, and actual historical paired-depth reconciliation. Report evidence and denominators rather than relying on the test count.

### Benchmarks and optimization - Not started

- `src/bench/README.md` describes a planned harness, but no measurement implementation or measured results were found in the ZIP. Freeze workloads and method, benchmark baseline vs optimized variants on Ayush's hardware, and report p50/p99/p99.9, throughput, sample counts, repetitions, variability, allocations, and hardware/compiler details. No performance numbers belong on the resume before measurement.

### Repo, report, and demo - Partial

- **Present:** Private GitHub repo with M1 source and PLAN.md/STATUS.md at `729d64a`; M1 ZIP remains in Drive. Local CLI, semantics, and validation changes are not yet pushed.
- **Open:** Continue tested incremental pushes under Ayush's author identity alone and tell him what changed. Finish the one-command CLI, reproducibility artifacts and audit script, honest `REPORT.md`, and a visually verified depth-animation demo. Use a clearly labeled synthetic public trace unless historical-sample display/redistribution rights are verified. Do not call it a live exchange feed. No deployment or report has been verified for this project.

### Learning notes - Partial

- **Present:** `docs/walkthrough.md` in the ZIP starts from bids, asks, spread, and price-time priority, then maps ideas to code and interview questions.
- **Open:** Ayush works through and explains the existing code himself; add feed semantics (L1/L2/L3, visible/hidden, queue position), latency measurement limits, and the difference between simulated fills and observed feed changes as those components are validated.

## Update rule

Update this file whenever a project file, implementation, test result, benchmark, report, demo, or repo state changes. For each change: identify the exact artifact/commit or URL, state what was verified and what is still unverified, update the relevant component status, then append a dated changelog entry. Record failed tests and abandoned experiments too. A change in a local ZIP is not a GitHub update, and a passing CI badge is not a measured latency result. Keep PLAN.md requirements and this status separate: the plan says what to do; this document says what is actually done.

## Changelog

| Date | Change | Evidence / remaining gap |
| --- | --- | --- |
| 2026-09-29 | Created the living status document. | Inspected the M1-complete Drive ZIP and private empty GitHub repository. Initial test count was not a test run. |
| 2026-09-29 | Ayush approved PLAN.md and asked for full execution and demo today, progress commits, a Brag video, and public repo at completion. Audited M1 and reran baseline tests. | Fresh CMake Release build in agent workspace (GCC 11.4): CTest 26/26 passed. Historical paired-depth, sanitizer, and hardware-specific performance checks still open. No code change/push yet. |

| 2026-09-29 | Fresh baseline sanitizer run and GitHub push credential prepared. | CMake Debug with ASan/UBSan in agent workspace: CTest 26/26 passed. GitHub classic token stored in a separate agent vault entry; no value committed. |
| 2026-09-29 | First GitHub push: M1 history plus approved plan/status at `729d64a`. | Readback from GitHub confirmed STATUS.md and private repository. |
| 2026-09-29 | Local CLI and historical feed semantics built; sample-validation limits investigated. | Release and ASan/UBSan CTest 28/28; synthetic CLI emits three fills. Official AMZN 10-level zip SHA-256 `5cff62a609b27aef82285382ad646c4c2facc0a692a60bedda633de64c4aa54f`: 269,748 paired rows; naïve full-depth 148 exact, 269,600 mismatch. Eligible same-price deltas 162,283/162,283 exact; 2,444/2,444 hidden executions unchanged; 105,020 other transitions excluded. This check does not prove full reconstruction. Local changes pending commit/push. |
