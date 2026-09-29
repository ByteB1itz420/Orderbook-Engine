# Brag plan: Orderbook Engine

## What is this project?
A C++20 price-time matching simulator with a synthetic event-by-event depth explorer, tests, historical message consistency checks, and a measured baseline/compact comparison.

## The angle
Show the actual browser explorer stepping through a synthetic matching example. No invented exchange-speed, trading, or full-history claims. This is a short visual introduction that sends the viewer to the README and walkthrough for the details.

## Hook
“Price first. Time next.” enters over the live site's dark, mint-accented hero.

## User flow
Open hero → show queued bid/ask at event 5 → show a crossing buy at event 8 → show tape and final depth at event 15. Scenes must show actual UI and real synthetic trace states, not abstract diagrams.

## Tone / format
Polished and restrained; 1920×1080 landscape; 20 seconds. Use #10141a background, #ecf1ed text, #8fe5cb mint and #f8a999 coral from the deployed site. Actual site screenshots preserve DM Sans and Instrument Serif; overlaid text uses a system sans. No voiceover requested.

## Storyboard
- 0–3.7s: Hook line “Price first. Time next.”, actual hero UI. Soft music opens.
- 3.7–7.4s: Actual explorer, queued asks and bids at event 5.
- 7.4–12.65s: Event 8: buy crosses two same-price resting asks, trade tape shows makers #103 and #105. Hold so a viewer can read it.
- 12.65–16.35s: Event 15 and synthetic trace label; no unsupported latency boast.
- 16.35–20s: “Read the code. Run the tests. Explain the book.” Repo name and URL, clean end card.

## Audio
Music bed: original synthesized ambient instrumental, kept below spoken-volume range; four soft synthesized arrival accents. Fade under final card. Keep visual readability over beat sync; no strobing, no narrator.

## Share copy
A C++20 order book built to be tested and explained. Step through a synthetic price-time matching trace, then read the measured tradeoffs in the repo.
