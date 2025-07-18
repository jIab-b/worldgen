#!/usr/bin/env python3
import os, struct
from pathlib import Path

CHUNK_X = 128
# Replace with your preset’s max_height:
MAX_HEIGHT =  (Path("data/chunks/chunk_0_0.npy").read_bytes()[8:12])
# For a quick hack, just hardcode the H you saw in step 3:
MAX_HEIGHT =  70

def check_raw(path, channels=1):
    size = path.stat().st_size
    expected = CHUNK_X * MAX_HEIGHT * CHUNK_X * channels
    print(f"{path.name}: {size} bytes, expected {expected}")
    assert size == expected, "size mismatch!"

def main():
    out = Path("data/chunks")
    raws = []
    for p in out.glob("chunk_*_0_0.raw"):
        raws.append(p)
    for p in raws:
        chan = 3 if "_color" in p.name else 1
        check_raw(p, chan)
    print("All raw files look sane.")

if __name__ == "__main__":
    main()