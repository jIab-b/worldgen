# Implementation Plan: Client-Side Procedural World Generator and Viewer

## 1. Overview
Define a fully-client side world generation and rendering pipeline in C++/WASM + WebGPU + raylib. The server’s role is reduced to:
- Accepting generation requests (seed + preset metadata)
- Storing or forwarding a detailed Chrome-trace JSON produced by the client
- Serving only static assets (HTML, JS/WASM bundles)

This plan outlines architecture, modules, build steps, integration points, and milestones.

## 2. High-Level Architecture

Browser (WASM)                             | Server (FastAPI)
------------------------------------------- | ---------------------------------------
raylib viewer (HTML5)                       | POST /submit-trace → receives trace.json
WASM “GenDLL” (C++ core) ↔ WebGPU via WGSL    | Serves HTML/JS/WASM over HTTP
Chrome-trace exporter                      | (No Python generation logic)
                                            |
*Data Flow:* Client generates world → renders → collects trace → submits trace back to server.

## 3. Deterministic Seeding & RNG

- **Seed format:** 128-bit (two uint64) via `XXH3_128(userString + "--world--" + version)`
- **Per-module streams:** split with SplitMix64 or PCG jump
- **On-disk/network header:** embed full 128-bit seed for reproducibility
- **Versioning:** each module uses its own version tag (e.g. `"Heightmap_v2"`)

## 4. Core Header-Only Modules

1. **Tracing.hpp**
   - `TRACE_SCOPE(name, args)` RAII JSON events
   - `TRACE_VALUE(key, value)` immediate log
2. **Random.hpp**
   - PCG64/PCG64Fast wrappers
   - Hash utilities for coordinate → seed

3. **Heightmap.hpp**
   - Multi-octave FBM GPU kernel (WGSL) with configurable lacunarity, gain, and octaves  
   - Hierarchical GPU min/max reduction to normalize height outputs  
   - Hydraulic erosion: simulate water flow, sediment transport & deposition via ping-pong textures  
   - Thermal erosion: apply slope-based material transport for realistic cliffs  
   - Edge tiling: overlapping borders to ensure seamless chunk stitching between adjacent chunks  
   - CPU fallback pipeline (NumPy/CuPy) for offline processing and debugging  

4. **Biomes.hpp**
   - Inputs: height, slope, humidity, and temperature textures  
   - Humidity map: noise-based moisture sampling + GPU diffusion (separable convolution)  
   - Temperature map: latitude gradient modulated by elevation and noise perturbation  
   - CPU classification pass:  
     - Sample per-texel inputs, apply user-defined threshold/curve tables → biome ID  
     - Support custom biome transition curves (e.g. desert↔oasis, tundra↔forest)  
   - Outputs: uint8 2D biome ID map + parameter textures (albedo, roughness, vegetation density)  

5. **Features.hpp**
   - `IFeature` interface: `void apply(ChunkCtx&)`  
   - **Caves**  
     - 3D FBM SDF kernel in WGSL with multi-frequency blending  
     - Gaussian-blur compute pass to smooth tunnel geometry  
   - **Rivers**  
     - CPU river pathfinding along steepest descent; carve channels in SDF heightmap  
     - GPU depth-filling SDF pass to form riverbeds  
   - **Volcanoes**  
     - Poisson-disc vent placement; radial SDF height perturbation kernel  
     - Lava flow SDF domain fill and thermal glow parameter textures  
   - Custom features: register via `FeatureRegistry.add<MyFeature>("Name_vX")`  
   - Execution order: sorted by priority tags to resolve overlapping correctly  
   - Debug overlays: GPU shaders to visualize individual SDF feature masks  

6. **TextureSynth.hpp**
   - Triplanar UV blending:
     - Sample three axis-aligned textures (XY, YZ, ZX)
     - Compute blend weights from the vertex normal (|nx|, |ny|, |nz|)
   - Virtual texturing:
     - Tile large (4K×4K) atlases divided into 256×256 pages
     - Stream pages on-demand via WebGPU bind groups
   - WGSL compute kernels:
     - Albedo atlas blending using biome parameters (albedo, tint)
     - Normal map synthesis (analytical normal + detail normal layers)
     - Ambient occlusion baking in screen-space SDF
     - Procedural roughness & metallic map generation per biome
   - CPU-side responsibilities:
     - Aggregate texture descriptors and build bind group layouts
     - Manage texture page requests and caching

7. **MeshTiler.hpp**
   - Marching Cubes Compute Pass:
     - TriTable stored as a 256-entry uniform array
     - Evaluate SDF per cell, generate edge intersections
     - Perform a prefix-sum scan in WGSL to allocate/compact the vertex list
   - Index Buffer Construction:
     - Build a 32-bit IBO with local chunk vertex offsets
   - CPU Mesh Assembly:
     - Read back GPU vertex & index buffers
     - Interleave attributes (position, normal, UV) into a single VBO
     - Construct a raylib Mesh struct for GPU submission
   - Skirt & Crack Fixing:
     - Extrude border vertices downward to hide gaps between chunks
     - Insert degenerate triangles at LOD boundaries to prevent cracks
   - LOD Morphing:
     - Blend mesh data between adjacent LOD levels for smooth transitions
   - Mesh Caching:
     - Hash meshes by chunkID+LOD and reuse GPU buffers on camera revisit

## 5. Generation Pipeline (Per Chunk)

```cpp
void GenerateChunk(ChunkID id, GPUContext& gpu) {
  TRACE_SCOPE("GenerateChunk", id);
  auto hm = Heightmap::create(id, gpu);
  if (settings.erosion) hm.erode(gpu);
  auto bm = Biomes::classify(hm);
  auto sdf = Caves::build(id, gpu);
  auto mesh = MeshTiler::from(hm, sdf, gpu);
  auto tex  = TextureSynth::make(hm, bm, gpu);
  Viewer::submit(mesh, tex);
}
```

- **GPU passes:** noise → erosion → cave SDF → triangle table marching cubes → texture synth
- **CPU passes:** biome classification, mesh assembly, trace logging

## 6. Tracing & Debugging

- Single ring buffer in WASM
- Copy to JS each frame → download or POST to `/submit-trace`
- View in `chrome://tracing`
- Build toggle: `-DPROCGEN_TRACE=1`

## 7. WebGPU Integration

- Use Dawn / wgpu-native via Emscripten ≥3.2
- Compile WGSL at runtime
- Double-buffer large textures (ping-pong)
- Tri-table as 256-entry uniform + in-place prefix sum

## 8. raylib Viewer Integration

- Minimal raylib port (audio, input, camera)
- Thin C API in GenDLL to expose `GenerateChunk` and `SubmitMesh`
- WASM glue (`raylib_viewer.js`) loads both viewer and generator modules
- UI widgets for preset editing and live parameter tweaking

## 9. Server API Adjustments

- **POST /generate** → returns HTML/JS/WASM entrypoint URL + initial config
- **POST /submit-trace** → accepts Chrome-trace JSON, stores to `data/traces/` with UUID
- **GET /data/** → static hosting of asset bundles, WASM, shaders

## 10. Build & Deployment Steps

1. Install Emscripten (≥3.2) and Dawn SDK
2. emcmake cmake . -DRAYLIB_WEB=ON -DPROCGEN_TRACE=(0|1)
3. emmake make → `viewer.html`, `main.wasm`, `main.js` (~3 MB gzipped)
4. Host server container: FastAPI serves `/` + `/data` + trace endpoint
5. CI pipeline: lint C++ headers, run JS unit tests, validate WASM output size

## 11. Milestones & Timeline

| Milestone                          | Deliverable                    | Estimate |
|------------------------------------|--------------------------------|----------|
| Prototype Heightmap + Viewer       | WASM heightmap demo            | 1 week    |
| Add Biome classification           | CPU biome map over heightmap   | 1 week    |
| Integrate Caves (SDF + MC)         | Visible cave meshes            | 2 weeks   |
| Texture synthesis & UI tweaks      | Triplanar textures & controls  | 1 week    |
| Tracing & debug UI                 | Chrome trace integration       | 1 week    |
| Server trace endpoint & deploy     | `/submit-trace` + Docker image | 1 week    |

## 12. Future Extensions

- Rivers & flooding modules
- Procedural roads/cities
- Dynamic weather/effects via IFeature
- Multiplayer synchronization (seed + trace) 