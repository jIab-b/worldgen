#!/usr/bin/env python3
import argparse
import uuid
from .preset import load_preset
from .generator import start_world


def main():
    parser = argparse.ArgumentParser(
        description="Generate a voxel world from a JSON preset"
    )
    parser.add_argument(
        "-p", "--preset",
        required=True,
        help="Path to the JSON preset file"
    )
    parser.add_argument(
        "-i", "--id",
        default=None,
        help="Optional run identifier (defaults to a UUID)"
    )
    args = parser.parse_args()

    # Load and validate the preset
    preset = load_preset(args.preset)
    # Choose or generate a run ID for the trace filename
    run_id = args.id or uuid.uuid4().hex
    # Generate the world (writes data/chunks/*.npy and data/traces/<run_id>.trace.json)
    trace_path = start_world(preset, run_id)
    print(f"World generated. Trace saved to: {trace_path}")


if __name__ == "__main__":
    main() 