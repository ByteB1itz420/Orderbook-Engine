# data

Small committed samples live here; large files go in `data/large/` (gitignored).

## Sources
- LOBSTER free samples: https://lobsterdata.com/info/DataSamples.php
  (message + orderbook CSVs; full academic data needs a waiver, samples are enough for v1)
- Crypto L2/L3: record your own (e.g. Binance diff-depth via websocket) for a large, free test set.

## LOBSTER message CSV columns
time, type, order_id, size, price, direction
- type 1: new limit order -> AddOrderEvent
- type 2: partial cancel, type 3: full delete -> CancelOrderEvent
- type 4: execution (visible), type 5: hidden execution -> ExecutionEvent
- direction: +1 buy, -1 sell
- price: dollars * 10000, already integer ticks
