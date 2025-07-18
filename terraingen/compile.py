#!/usr/bin/env python3
import subprocess
import glob
import sys
import os

# Build script for terraingen C++ → WASM module (pure compute, no raylib)

def main():
    # Resolve directories relative to script location
    script_dir = os.path.abspath(os.path.dirname(__file__))
    src_dir = os.path.join(script_dir, 'src')
    src_files = glob.glob(os.path.join(src_dir, '*.cpp'))
    if not src_files:
        print('No source files found in terraingen/src/')
        sys.exit(1)

    emcc = 'emcc'
    cflags = [
        '-std=c++17', '-O3',
        '-DPROCGEN_TRACE=1',
        '--closure', '1',
        '-s', 'WASM=1',
        '-s', 'ALLOW_MEMORY_GROWTH=1',
        '-s', 'USE_WEBGPU=1',
        '-s', 'EXPORTED_FUNCTIONS=["_GenerateChunk"]',
        '-s', 'EXPORTED_RUNTIME_METHODS=["cwrap","getValue"]'
    ]
    include_dir = os.path.join(script_dir, 'include')
    includes = [f"-I{include_dir}"]
    output_file = os.path.join(script_dir, 'terraingen.wasm')
    output = ['-o', output_file]

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