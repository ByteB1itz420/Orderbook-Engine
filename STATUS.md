# Orderbook-Engine: project status

**As of:** September 29, 2026  
**State:** Execution authorized by Ayush at 12:07 IST on September 29, 2026. He asked for the full PLAN.md build and a deployed demo today, with progress commits, a Brag video, and this repository public at the end. Work has begun with the M1 audit. Remaining tasks below are not done merely because the deadline is today.

**Where the work is:** The [private GitHub repository](https://github.com/ByteB1itz420/Orderbook-Engine) contains the M1 source, later engineering work, measured report, and deployed demo through commit `c56feec`. An [M1-complete ZIP in Ayush's BITS Drive](https://drive.google.com/file/d/1WNCGPdEwKJRlnZiwoQChqBPA0v3rVmCg/view?usp=drivesdk&authuser=f20230965%40goa.bits-pilani.ac.in) contains actual C++20 source, CMake configuration, tests, CI configuration, and a beginner walkthrough. The ZIP had two local commits; both were included in the first GitHub push. New work under PLAN.md has begun with an audit and clean build; later engineering work and a deployed demo have since been pushed.

**Evidence boundary:** This snapshot comes from inspecting the Drive ZIP, checking the live GitHub repo, and building/testing on an agent workspace, not from Ayush's machine. The ZIP has 26 initial `TEST_CASE` declarations, and all 26 passed in a fresh Release build using GCC 11.4 on the agent workspace. The same 26 tests passed in a fresh Debug build with ASan/UBSan; five new tests also pass in Release and ASan/UBSan (31/31). GitHub CI and Ayush’s local environment have not yet been rerun. The original M1 README called itself complete; the replay CLI was then a stub and has since been implemented. This log follows inspected code, not broad milestone labels.

## Component progress

Use **Present in M1**, **Partial**, **Not started**, or **Blocked**. A feature is not complete merely because an interface, comment, test stub, or plan mentions it. Check PLAN.md for requirements and keep both files aligned when scope changes.

### Matching engine and data structures - Partial

- **Present in M1 ZIP:** C++20 `OrderBook` implementation and tests for price-time matching, partial fills, market/limit/IOC behavior, cancel, reduce, replace, and optional self-trade prevention. It uses integer prices, `std::map` price levels, `std::deque` FIFO orders, and `std::unordered_map` ID lookup as a clear baseline. An `OrderPool` class exists separately.
- **Open:** Audit exact semantics and edge cases with Ayush; write the frozen contract; decide modify priority and capacity behavior; implement and compare stable pool-backed intrusive FIFO, flat ID lookup, and any justified cache-aware change. The current baseline's cancellation scans a price-level deque, and the pool is not integrated with the `OrderBook`; no verified zero-allocation hot-path claim. A separate fixed-capacity `CompactBook` alternative has been built and compared on 5,000 seeded mixed events against the baseline, with exact best quotes, counts, and trade records; broader contract parity remains open.

### Historical replay and synthetic flow - Partial

- **Present in M1 ZIP:** LOBSTER-format line parser and a `ReplayEngine` that applies add, cancel, reduce, replace, and visible execution events. Tests replay a **hand-written 10-line CSV fixture**, not an official historical LOBSTER sample. Hidden executions do not change visible depth.
- **Open:** Obtain/verify the official paired message and orderbook sample, establish starting state and supported-event policy, compare reconstructed depth row by row with counts and mismatches, record hashes/provenance, and separate historical observed updates from synthetic incoming orders. `src/replay/main.cpp` now has synthetic and historical modes, locally built and tested; paired-depth validation remains a narrow delta check, not a full book check. Do not claim real historical validation from the hand-written fixture.

### Correctness - Partial

- **Present in M1 ZIP:** 26 declared Catch2 test cases, including matching, parser, hand-written replay, determinism, and randomized reference-model coverage; CMake test setup and CI workflow with GCC/Clang plus ASan/UBSan configuration.
- **Open:** Run on Ayush's environment and check CI after a push. Add explicit per-event integrity/conservation invariants, failure-seed shrinking and saved golden traces, broader boundary cases, and actual historical paired-depth reconciliation. Report evidence and denominators rather than relying on the test count.

### Benchmarks and optimization - Partial

- A deterministic harness, frozen protocol and raw agent-host result CSV are committed. The first agent-host runs show mixed results: the compact alternative improves cancel but is slower on other operations. These are not Ayush-laptop measurements or publishable headline latency yet. Freeze and commit the harness protocol before a reported run, record hardware/compiler and raw results, measure allocations separately, and later rerun on his laptop. No performance numbers belong on the resume before that validation.

### Repo, report, and demo - Partial

- **Present:** Private GitHub repo through `c56feec` with M1 source, CLI, semantics, validation, measured report, and a deployed public synthetic demo. The official sample bytes remain uncommitted. A post-deployment Brag video has been rendered locally and inspected; its push and public repository release are pending.
- **Open:** Push the video and source, run the public-release audit, verify the video link after push, then make the repository public. Vercel is deployed independently of GitHub. Allocation instrumentation, Ayush-laptop measurements and the full interview rehearsal are separate follow-ups.

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
| 2026-09-29 | Local CLI and historical feed semantics built; sample-validation limits investigated. | Release and ASan/UBSan CTest 28/28; synthetic CLI emits three fills. Official AMZN 10-level zip SHA-256 `5cff62a609b27aef82285382ad646c4c2facc0a692a60bedda633de64c4aa54f`: 269,748 paired rows; naïve full-depth 148 exact, 269,600 mismatch. Eligible same-price deltas 162,283/162,283 exact; 2,444/2,444 hidden executions unchanged; 105,020 other transitions excluded. This check does not prove full reconstruction. Committed and pushed at `61b7940`. |

| 2026-09-29 | Local compact-book alternative and benchmark protocol added; not yet pushed. | Fixed-capacity node/level arrays and flat ID table, seeded 5,000-event differential test. Release and ASan/UBSan: 30/30 passed. Agent-host benchmark is now reported with committed method and raw CSV, with slower compact add/match; allocation check and user-machine rerun remain open. |

| 2026-09-29 | Local measured report and reproducibility manifest prepared. | `REPORT.md`, `results/agent_host_benchmark.csv`, `results/manifest.sha256`, and `results/audit.py`; audit passes. Agent-host median p99 across five runs: baseline add/cancel/modify/match 135/131/268/93 ns; compact 273/62/221/177 ns. Measured host is not Ayush laptop; capacity and STP differ. Pending commit/push. |

| 2026-09-29 | Deployed first interactive synthetic explorer at https://orderbook-engine-demo.vercel.app. | Vercel project `orderbook-engine-demo`, production alias returned by CLI; public HTTP 200 and JSON has 15 synthetic events. Inspected desktop 1280px and mobile 390px screenshots: readable panels and responsive layout. Browser interaction test: 15/15, eight fills, back and 1.5x speed work, no horizontal overflow/page errors. Vercel SSO protection disabled for public demo; repo still private, site source not yet pushed, video pending. |
| 2026-09-29 | Fixed explorer copy for a fully filled event and redeployed the public demo; produced post-deployment Brag/Hyperframes walkthrough video. | Live site [orderbook-engine-demo.vercel.app](https://orderbook-engine-demo.vercel.app): event 8 now reads "Fully filled; nothing remains on the book." Browser verification at 390px and 1280px: 15/15, eight fills, back and speed controls, zero page errors/horizontal overflow. `brag-output/brag.mp4` is 20s, 1920x1080 H.264/AAC with original synthesized music, real explorer captures at events 5/8/15, and event-8 poster baked into frame zero. Hyperframes check: zero errors, 20/20 text contrast checks; visual contact sheet and settled poster inspected. Pending commit, push, public-release audit. |
