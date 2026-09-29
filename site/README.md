# Interactive explorer

Static HTML/CSS/JS, no backend and no API calls except loading `data.json`. The trace is deterministic and **synthetic**, not a market feed. Price values in trace are display cents; the C++ core accepts integer ticks with unit defined per input source. Playback uses local browser memory only. `app.js` renders event history rather than changing user data.

The example was hand-checked against the matching rules. `../tests/test_demo_trace.cpp` replays the same commands in the C++ baseline and checks generated depth and fills against `data.json`. Site source is part of the repository; Vercel deployment is independent of Git auto-deploy.
