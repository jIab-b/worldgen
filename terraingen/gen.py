#!/usr/bin/env python3
"""
gen.py: convenience script to compile Terraingen CLI, generate a chunk, and optionally build the WASM bundle.
"""
import subprocess
import sys
import os
import argparse

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))


def run_native(cx, cz, outdir):
    # Build native CLI
    print('Building native CLI...')
    subprocess.check_call([sys.executable, 'compile.py', '--native'])
    # Determine executable path
    exe_name = 'terraingen'
    if os.name == 'nt':
        exe_name += '.exe'
    exe_path = os.path.join(SCRIPT_DIR, exe_name)
    # Generate chunk
    cmd = [exe_path, str(cx), str(cz), '--outdir', outdir]
    print('Generating chunk via native CLI:', ' '.join(cmd))
    subprocess.check_call(cmd)
    print(f'Chunk {cx},{cz} written to {outdir}')


def run_wasm():
    # Build WASM bundle
    print('Building WebAssembly bundle...')
    subprocess.check_call([sys.executable, 'compile.py', '--wasm'])
    print('WASM build complete. To run in browser:')
    print(f'  cd {SCRIPT_DIR}')
    print('  emrun --browser chrome terraingen.html  # or python3 -m http.server 8000')


def main():
    parser = argparse.ArgumentParser(description='Compile, generate chunk, and optionally build WASM bundle')
    parser.add_argument('cx', type=int, help='Chunk X coordinate')
    parser.add_argument('cz', type=int, help='Chunk Z coordinate')
    parser.add_argument('--outdir', default=os.path.join('..', 'viewer', 'chunks'),
                        help='Output directory for chunk files')
    parser.add_argument('--wasm', action='store_true', help='Also build the WASM bundle')
    args = parser.parse_args()

    # Ensure output directory exists
    os.makedirs(args.outdir, exist_ok=True)

    run_native(args.cx, args.cz, args.outdir)

    if args.wasm:
        run_wasm()


if __name__ == '__main__':
    main() 