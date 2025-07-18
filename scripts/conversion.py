#!/usr/bin/env python3
"""
Convert NumPy .npy chunk files to plain raw byte files for the Raylib Web viewer.
"""
import argparse
from pathlib import Path
import shutil

def main():
    parser = argparse.ArgumentParser(
        description="Convert .npy chunk files to .raw for the viewer"
    )
    parser.add_argument(
        "-i", "--input-dir",
        type=Path,
        default=Path("data/chunks"),
        help="Directory containing .npy chunk files"
    )
    parser.add_argument(
        "-o", "--output-dir",
        type=Path,
        default=Path("viewer/chunks"),
        help="Directory to write .raw files"
    )
    args = parser.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)

    for npy_path in sorted(args.input_dir.glob("chunk_*.npy")):
        raw_path = args.output_dir / (npy_path.stem + ".raw")
        print(f"Copying {npy_path} -> {raw_path}")
        # Memmap .npy files have no header and are raw data; copy bytes directly
        shutil.copyfile(npy_path, raw_path)

if __name__ == "__main__":
    main() 