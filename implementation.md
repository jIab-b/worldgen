# Comprehensive Implementation Plan for “Text-to-Voxel World v0.1”

## Overview
This document outlines a detailed implementation plan for building a text-to-voxel world generator MVP focused on terrain and biomes. The system uses Python with CUDA for compute-intensive operations (e.g., noise generation), WebGL with Three.js for rendering, and emphasizes modularity, simplicity, and extensibility. Inputs are pure data (JSON presets), outputs are deterministic, and extensions happen via registered functions without core changes. Function calls are traced and saved to JSON for reproducibility and ML learnability.

Key features:
- Terrain and biome generation using GPU-accelerated noise.
- Chunk-based voxelization for scalability.
- Optional function calls in presets for extensibility.
- Call trace registry for logging executions.
- Minimal codebase: Target <2k LoC, few files.

## 0. Guiding Principles
- **Pure-Data Inputs → Deterministic Outputs**: Everything driven by JSON presets and seeds; no hidden state.
- **Modularity**: Each subsystem (noise, biomes, etc.) in its own module, unaware of UI/ML.
- **Extensibility**: Add features via JSON files or registered functions; no core code changes.
- **Simplicity**: Minimal files/code; use lightweight libs (e.g., CuPy, FastAPI, Three.js).
- **One Source of Truth**: preset.json (user requests) + trace.json (executed calls).
- **Focus**: Start with terrain + biomes; extensible for caves, structures, etc.
- **Tech Stack**: Python/CUDA backend, WebGL/Three.js frontend.

## 1. Repository / File-System Layout
```
text-to-voxel/
├─ README.md                 # Build/run instructions, extension guide
├─ pyproject.toml            # Poetry/hatch for deps (CuPy, Numba, FastAPI)
├─ requirements.txt          # Fallback deps list
│
├─ worldgen/                 # Backend library (~1k LoC)
│  ├─ __init__.py
│  ├─ config_schema.py       # Pydantic models for Preset/BiomeConf
│  ├─ preset.py              # Load/save/validate Preset objects
│  ├─ noise.py               # CuPy/Numba CUDA kernels (value noise, fBm)
│  ├─ biome.py               # BiomeMask generation
│  ├─ heightmap.py           # Per-biome terrain synthesis
│  ├─ voxelizer.py           # Chunk-based 3D voxel filling
│  ├─ api.py                 # Function registry (@register_api decorator)
│  ├─ trace.py               # CallTrace class for logging calls to JSON
│  ├─ generator.py           # Orchestrates generation flow
│  └─ utils.py               # Helpers (e.g., hashing, array utils)
│
├─ server/                   # API service layer (~200 LoC)
│  ├─ __init__.py
│  ├─ rest_api.py            # FastAPI endpoints (/generate, /preview, /trace)
│  └─ ws.py                  # Optional WebSocket for chunk streaming
│
├─ viewer/                   # Static WebGL/Three.js frontend
│  ├─ index.html             # Main entry point
│  ├─ app.js                 # Three.js scene setup, voxel rendering
│  ├─ shaders/               # GLSL shaders
│  │   ├─ voxel.vert
│  │   └─ voxel.frag
│  └─ libs/three.module.js   # Three.js library
│
├─ data/
│  ├─ presets/               # User/LLM-generated *.json presets
│  └─ traces/                # *.trace.json call logs
│
└─ tests/                    # Pytest suite
```

**Implementation Notes**:
- Keep directories flat; avoid sub-subfolders.
- Data files are the "database"—no external DB needed.

## 2. Core Abstractions (worldgen/config_schema.py)
Use Pydantic for validation and JSON schema export.

```python
from pydantic import BaseModel, Literal

class BiomeConf(BaseModel):
    id: str                  # e.g., "plains"
    band: Literal["equator", "temperate", "polar"]
    width: float             # 0-1 normalized
    base: int                # Base height
    amp: int                 # Amplitude for hills
    freq: float              # Noise frequency

class Preset(BaseModel):
    world_size: int          # e.g., 2048
    sea_level: int = 62
    macro_seed: int
    height_seed: int
    biomes: list[BiomeConf]
    calls: list[dict] = []   # Optional: [{"fn": "carve_cave", "args": {...}}]
```

- Preset auto-validates on load (e.g., via preset.py).
- Export schema for ML: `Preset.model_json_schema()`.

## 3. API / Function Registry (worldgen/api.py)
Simple decorator-based registry for extensible functions.

```python
_REGISTRY: dict[str, Callable] = {}
_SCHEMA: dict[str, dict] = {}  # JSON schema per function

def register_api(name: str, schema: dict):
    def decorator(func):
        _REGISTRY[name] = func
        _SCHEMA[name] = schema
        return func
    return decorator

# Example usage in a new module (e.g., caves.py):
@register_api("carve_cave", {"type": "object", "properties": {"seed": {"type": "integer"}}})
def carve_cave(vox_array, seed):
    # Implementation...
    pass
```

- During generation, lookup and execute via registry.
- Expose `/schema` endpoint in server for aggregated schemas (for ML fine-tuning).

## 4. Call Trace Registry (worldgen/trace.py)
Logs called functions + args to JSON.

```python
import json
from pathlib import Path

class CallTrace:
    def __init__(self):
        self.calls = []

    def record(self, fname: str, kwargs: dict):
        self.calls.append({"fn": fname, "args": kwargs})

    def save(self, path: str):
        Path(path).write_text(json.dumps(self.calls, indent=2))

    def load(self, path: str):
        self.calls = json.loads(Path(path).read_text())
```

- Integrated into generator: Record after each call execution.
- Saves to `data/traces/{preset_id}.trace.json`.

## 5. GPU Kernels (worldgen/noise.py)
CUDA-accelerated noise functions.

- Use CuPy for GPU (e.g., `cupy.ndarray`).
- Fallback: Numba `@njit(parallel=True)` for CPU.
- Key functions:
  - `value_noise2D(seed, xs, zs) -> cupy.ndarray`
  - `fbm_2D(seed, xs, zs, freq, octaves) -> cupy.ndarray`

Implementation: Port standard Perlin/value noise to CuPy kernels. Ensure deterministic (seed-based).

## 6. Biome and Heightmap Generation
- **biome.py**: Generate BiomeMask (2D array) based on biomes' bands/widths. Use macro_seed for procedural banding.
- **heightmap.py**: For each biome, compute height = base + amp * fbm(...) on masked indices.
  - Parallelize via CuPy slicing.

## 7. Chunk-Based Voxelization (worldgen/voxelizer.py)
To handle large worlds scalably, process in chunks (e.g., 128x128x128).

- Divide world into chunks: e.g., for world_size=2048, 16x16x(height/128) chunks.
- For each chunk:
  - Map heightmap to 3D: Fill voxels below height with terrain blocks; apply biome materials.
  - Use memory-mapped NumPy (`np.memmap`) for low-RAM handling.
  - Parallelize chunk processing with CuPy streams or multiprocessing.

```python
def build_chunks(preset, height, mask):
    chunk_size = 128
    vox = {}  # Dict of chunk_id: np.memmap
    for cx in range(0, preset.world_size, chunk_size):
        for cz in range(0, preset.world_size, chunk_size):
            # Slice height/mask for chunk
            # Fill 3D array (y=0 to max_height)
            # Apply sea_level, biome-specific blocks
            vox[(cx, cz)] = np.memmap(...)  # Save to disk
    return vox
```

- Output: Chunk files or in-memory dict for API serving.

## 8. Generator Orchestration (worldgen/generator.py)
Ties it all together.

```python
def start_world(preset: Preset, trace: CallTrace):
    mask = BiomeMask(preset).build()  # GPU
    height = HeightField(preset, mask).build()  # GPU
    vox = Voxelizer(preset, height, mask).build_chunks()  # Chunked
    for call in preset.calls:
        fn = _REGISTRY[call["fn"]]
        fn(vox, **call["args"])  # Apply to voxels
        trace.record(call["fn"], call["args"])
    trace.save(f"data/traces/{preset_id}.trace.json")
    return vox
```

## 9. Server Layer (server/rest_api.py)
FastAPI for endpoints:
- POST /generate: Accept preset JSON, run start_world, return chunk URLs or previews.
- GET /preview: Return height/biome PNGs.
- GET /trace: Return trace.json.
- Optional WebSocket: Stream chunk updates.

## 10. Viewer (viewer/app.js)
Use Three.js with WebGL for rendering.
- Fetch preview PNGs, decode to textures.
- Build scene: Instanced meshes for voxels (one per block type).
- Shaders: Basic voxel.vert/frag for lighting/texturing.
- Interactivity: Orbit controls, "play trace" button to poll and replay updates.
- Fallback to WebGL for broader compatibility (no WebGPU-specific code).

## 11. ML Integration
- Expose /schema for Preset + registered functions.
- scripts/make_dataset.py: Generate random presets, render thumbnails, auto-caption for fine-tuning.

## 12. Extending the System
- Add new feature: Create module (e.g., villages.py), @register_api a function, add to preset.calls.
- No core changes needed.

## 13. Build / Run Flow
1. `pip install -r requirements.txt` (installs CuPy—requires CUDA toolkit).
2. `python -m server.rest_api` (starts FastAPI on :8000).
3. Open `viewer/index.html` in browser.
4. POST preset to /generate; view in app.

## 14. Testing
- tests/test_generator.py: Assert deterministic outputs for tiny worlds.
- tests/test_trace.py: Verify call logging.
- No GPU required for core tests (use Numba fallback).

## 15. Roadmap
- Week 1: Scaffolding, schema, noise kernels.
- Week 2: Biome/heightmap, chunked voxelizer.
- Week 3: API/trace, generator.
- Week 4: FastAPI server, Three.js viewer (WebGL).
- Week 5: Dataset script, ML hooks.
- Week 6: Tests, refinements, demo.

## 16. LOC Estimate
- worldgen/: ~1000
- server/: ~200
- viewer/ (JS): ~300
- Tests/scripts: ~300
Total: <2k LoC, solo-maintainable. 