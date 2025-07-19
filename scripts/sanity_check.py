#!/usr/bin/env python3
"""
Sanity-check script for chunk .bin mesh files.
Verifies that for each chunk_<cx>_<cz>_{vertices,indices}.bin in data/:
  - both files exist
  - vertices file size is divisible by 32 (8 floats × 4 bytes)
  - indices file size is divisible by 4 bytes
"""
import os
import sys
import re

# Path to data directory relative to this script
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir, "data"))

if not os.path.isdir(DATA_DIR):
    print(f"ERROR: Data directory not found: {DATA_DIR}")
    sys.exit(1)

pattern = re.compile(r'^chunk_(-?\d+)_(-?\d+)_(vertices|indices)\.bin$')
files = os.listdir(DATA_DIR)

chunks = {}
for fname in files:
    m = pattern.match(fname)
    if m:
        cx, cz, typ = m.group(1), m.group(2), m.group(3)
        key = (cx, cz)
        chunks.setdefault(key, {})[typ] = fname

errors = False
for (cx, cz), d in sorted(chunks.items()):
    if 'vertices' not in d:
        print(f"ERROR: Missing vertices file for chunk {cx},{cz}")
        errors = True
    if 'indices' not in d:
        print(f"ERROR: Missing indices file for chunk {cx},{cz}")
        errors = True
    if 'vertices' in d:
        path = os.path.join(DATA_DIR, d['vertices'])
        size = os.path.getsize(path)
        if size % (8 * 4) != 0:
            print(f"ERROR: {d['vertices']} size {size} not divisible by 32 bytes")
            errors = True
    if 'indices' in d:
        path = os.path.join(DATA_DIR, d['indices'])
        size = os.path.getsize(path)
        if size % 4 != 0:
            print(f"ERROR: {d['indices']} size {size} not divisible by 4 bytes")
            errors = True

if not chunks:
    print(f"WARNING: No chunk .bin files found in {DATA_DIR}")

if errors:
    sys.exit(1)

print("Sanity check passed: all chunk .bin files are present and valid.")