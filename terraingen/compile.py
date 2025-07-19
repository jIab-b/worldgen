#!/usr/bin/env python3
import subprocess
import glob
import sys
import os
import argparse
import shutil

# Build script for terraingen C++ → WASM module (pure compute, no raylib)

def main():
    # Resolve directories relative to script location
    script_dir = os.path.abspath(os.path.dirname(__file__))
    src_dir = os.path.join(script_dir, 'src')
    src_files = glob.glob(os.path.join(src_dir, '*.cpp'))
    if not src_files:
        print('No source files found in terraingen/src/')
        sys.exit(1)

    # Parse args for native vs wasm build
    parser = argparse.ArgumentParser(description="Build Terraingen CLI and/or WebAssembly bundle.")
    parser.add_argument('--native', action='store_true', help='Build native CLI binary')
    parser.add_argument('--wasm', action='store_true', help='Build WebAssembly HTML bundle')
    args = parser.parse_args()
    # Default to both if none specified
    if not args.native and not args.wasm:
        args.native = True
        args.wasm = True

    # Source files
    include_dir = os.path.join(script_dir, 'include')
    includes = [f"-I{include_dir}"]
    # Output an HTML+JS+WASM bundle for browser-based CLI execution
    output_file = os.path.join(script_dir, 'terraingen.html')
    # Generate HTML wrapper by specifying .html output
    output = ['-o', output_file]

    # Native build
    if args.native:
        cc = shutil.which('g++') or shutil.which('clang++')
        if not cc:
            sys.exit('Error: No C++ compiler found for native build')
        native_out = os.path.join(script_dir, 'terraingen')
        # Include header directory for native build
        native_cmd = [cc] + includes + src_files + [ '-std=c++17', '-O3', '-lstdc++fs', '-o', native_out ]
        print('Building native CLI:', ' '.join(native_cmd))
        subprocess.check_call(native_cmd)
        print(f'✔ Built native binary: {native_out}')

    # WASM build
    if args.wasm:
        # Ensure chunks directory exists for embedding
        chunks_dir = os.path.join(script_dir, 'chunks')
        os.makedirs(chunks_dir, exist_ok=True)
        emcc = shutil.which('emcc') or sys.exit('Error: emcc not found for WASM build')
        cflags = [
            '-std=c++17', '-O3',
            '-DPROCGEN_TRACE=1',
            '--closure', '1',
            '-s', 'WASM=1',
            '-s', 'ALLOW_MEMORY_GROWTH=1',
            '-s', 'USE_WEBGPU=1',
            # Embed WGSL shader files for WebGPU path
            '--embed-file', os.path.join(script_dir, 'shaders'),
            # Embed chunks directory so WASM can write output
            '--embed-file', os.path.join(script_dir, 'chunks'),
            '-s', 'EXPORTED_FUNCTIONS=["_GenerateChunk"]',
            '-s', 'EXPORTED_RUNTIME_METHODS=["cwrap","getValue"]'
        ]
        cmd = [emcc] + src_files + includes + cflags + output
        # On Windows emcc is a batch file; use shell=True with a command string
        cmd_str = ' '.join(f'"{arg}"' if ' ' in arg else arg for arg in cmd)
        print('Running:', cmd_str)
        if os.name == 'nt':
            subprocess.check_call(cmd_str, shell=True)
        else:
            subprocess.check_call(cmd)

if __name__ == '__main__':
    main() 