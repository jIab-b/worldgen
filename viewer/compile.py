#!/usr/bin/env python3
"""
Compile a C file that uses raylib to WebAssembly/HTML5 via Emscripten.

Example:
    python viewer/compile.py viewer/raylib_viewer.c
"""
import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

# ------------------------------------------------------------
# Helper functions
# ------------------------------------------------------------
def locate_emcc() -> str:
    """Return the full path to `emcc` or exit with a helpful message."""
    emcc = shutil.which("emcc")
    if emcc:
        return emcc
    sys.exit(
        "Error: `emcc` not found.  Run the Emscripten env script first or add "
        "EMSDK/upstream/emscripten to your PATH."
    )


def build_command(emcc: str, src: Path, dst: Path, libs_dir: Path) -> list[str]:
    """Compose the emcc command line."""
    return [
        emcc,
        str(src),
        "-o",
        str(dst),
        "-Os",                    # optimise for size
        "-DPLATFORM_WEB",
        "-I", str(libs_dir),      # headers
        "-L", str(libs_dir),      # lib directory
        "-lraylib.web",           # links libraylib.web.a
        "-s", "USE_GLFW=3",       # raylib's window/input backend
        "-s", "ASYNCIFY",         # allow blocking I/O (e.g. fetch)
        "-s", "WASM=1",           # generate .wasm
        # Embed the chunks directory so it’s available in the virtual filesystem
        "--embed-file", "chunks",
    ]


# ------------------------------------------------------------
# Main entry-point
# ------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(
        description="Compile a raylib C source file to WebAssembly/HTML."
    )
    parser.add_argument("source", help="Path to the .c file to compile")
    parser.add_argument(
        "-o",
        "--output",
        help="Output file (.html).  Defaults to <source>.html",
    )
    args = parser.parse_args()

    src_path = Path(args.source).resolve()
    if not src_path.is_file():
        sys.exit(f"Source file not found: {src_path}")

    out_path = (
        Path(args.output).with_suffix(".html").resolve()
        if args.output
        else src_path.with_suffix(".html")
    )

    script_dir = Path(__file__).parent.resolve()
    # Ensure chunks directory exists for embedding
    chunks_dir = script_dir / 'chunks'
    if not chunks_dir.exists():
        chunks_dir.mkdir()
    libs_dir = script_dir / "libs"

    emcc = locate_emcc()
    cmd = build_command(emcc, src_path, out_path, libs_dir)

    print("Running:", " ".join(cmd))
    subprocess.run(cmd, check=True)
    print(f"✔ Built {out_path}")


if __name__ == "__main__":
    main() 