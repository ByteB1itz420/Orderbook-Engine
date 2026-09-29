#!/usr/bin/env python3
"""Check same-price visible-size deltas in a paired LOBSTER 10-level sample.

This is NOT full L3 reconstruction: the sample starts after an unknown pre-window
state and filters events to displayed depth. Run locally on the two publisher CSVs.
"""
import argparse
import collections
import csv


def levels(row):
    values = list(map(int, row))
    if len(values) != 40:
        raise ValueError("Expected 10-level orderbook row (40 columns)")
    bid = {values[4*j+2]: values[4*j+3] for j in range(10) if values[4*j+3] > 0}
    ask = {values[4*j]: values[4*j+1] for j in range(10) if values[4*j+1] > 0}
    return bid, ask


def validate(messages, orderbook):
    counts = collections.Counter()
    examples = []
    previous = None
    with open(messages, newline="") as msg, open(orderbook, newline="") as dep:
        for n, (m, d) in enumerate(zip(csv.reader(msg), csv.reader(dep)), 1):
            kind, qty, price, side = int(m[1]), int(m[3]), int(m[4]), int(m[5])
            current = levels(d)
            counts["rows"] += 1
            if previous is not None:
                old = previous[0] if side == 1 else previous[1]
                new = current[0] if side == 1 else current[1]
                # Comparing one unchanged price level avoids boundary shifts.
                if kind in (1, 2, 3, 4) and price in old and price in new:
                    expected = qty if kind == 1 else -qty
                    actual = new[price] - old[price]
                    counts["eligible"] += 1
                    if actual == expected:
                        counts["matched"] += 1
                    else:
                        counts["mismatched"] += 1
                        if len(examples) < 5:
                            examples.append((n, kind, price, expected, actual))
                elif kind == 5:
                    counts["hidden"] += 1
                    if current == previous:
                        counts["hidden_unchanged"] += 1
                else:
                    counts["excluded_boundary_or_type"] += 1
            previous = current
        if msg.readline() or dep.readline():
            raise ValueError("Message and orderbook row counts differ")
    return counts, examples


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--messages", required=True)
    p.add_argument("--orderbook", required=True)
    args = p.parse_args()
    counts, examples = validate(args.messages, args.orderbook)
    for key in ("rows", "eligible", "matched", "mismatched", "hidden", "hidden_unchanged", "excluded_boundary_or_type"):
        print(f"{key}={counts[key]}")
    for example in examples:
        print("mismatch row,kind,price,expected_delta,observed_delta:", *example)
    raise SystemExit(1 if counts["mismatched"] or counts["hidden"] != counts["hidden_unchanged"] else 0)
