#!/usr/bin/env python3
"""Check local committed artifacts; external LOBSTER zip is downloaded separately."""
from pathlib import Path
import hashlib
root=Path(__file__).resolve().parents[1]
for line in (root/'results/manifest.sha256').read_text().splitlines():
    expected, relative=line.split('  ',1)
    if 'external publisher download' in relative:
        print('external sample: download and check manually:',expected)
        continue
    actual=hashlib.sha256((root/relative).read_bytes()).hexdigest()
    print('OK' if actual==expected else 'MISMATCH',relative)
    if actual!=expected: raise SystemExit(1)
