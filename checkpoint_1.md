
---

## GPU Acceleration Progress (as of Checkpoint 1.1)

### Completed GPU Work
1. **WebGPU Context**  
   • `GPUContext` now owns a `WGPUDevice` and queue when the browser supports WebGPU.  
   • Helpers to create R32F and R8Uint textures on GPU while still mirroring data in CPU vectors for fallback.
2. **Height-map Generation**  
   • WGSL compute shader dispatches `256×256/8×8` workgroups to fill the height texture with deterministic noise.  
   • CPU value-noise loop remains as a fallback path.
3. **Biome Classification (in progress)**  
   • `biome_classify.wgsl` committed.  
   • GPU path skeleton added to `Biomes::Classify()`; still needs pipeline + read-back wiring.

### To-do for Biome GPU Stage
- [ ] Create uniform buffer for low/mid altitude thresholds.  
- [ ] Build compute pipeline & bind group.  
- [ ] Dispatch and read back the R8Uint texture into `BiomeMap` for the CPU-based `TextureSynth` until that is ported.

---

## Remaining GPU-Acceleration Tasks (Roadmap)

1. **Finish Biome Classification wiring** (see to-do above).
2. **Texture Synthesis on GPU**  
   • Implement `triplanar.wgsl` (height, biome → albedo/normal/roughness RGBA8).  
   • Replace CPU loops; no read-back required.
3. **Feature SDF Passes**  
   • Port `SimpleCaves` carving loop to compute shader that writes directly into the SDF texture.  
   • Re-use same pattern for future rivers/volcanoes.
4. **3-D SDF & Marching Cubes**  
   a. WGSL kernel builds 64³ R16F distance field (height + caves).  
   b. `marching_cubes.wgsl` outputs compact vertex & index buffers via prefix-sum compaction.  
   c. `MeshTiler::Generate()` keeps GPU buffers; CPU path used only when WebGPU unavailable.
5. **Viewer Integration**  
   • Expose C API (`GetMeshBuffers`) returning `WGPUBuffer` handles.  
   • JS/raylib viewer imports buffers and draws with indirect commands; no CPU copy.
6. **Memory Utilities**  
   • Create pipeline cache, staging buffer allocator, and transient resource pool.  
   • One staging buffer for rare read-backs (e.g., trace screenshots).
7. **Build System**  
   • Migrate deprecated `-s USE_WEBGPU=1` to `--use-port=emdawnwebgpu`.  
   • Add `-DPROCGEN_GPU=0` switch for CI without WebGPU.

---

## Remaining Project Implementation Steps (including GPU targets)

Module | CPU Status | GPU Target | Notes
------ | ---------- | ---------- | -----
Heightmap | DONE | DONE | erosion kernels still missing
Biomes | CPU & GPU skeleton | Finish GPU, remove read-back later | humidity/temperature maps future work
TextureSynth | CPU placeholder | Triplanar WGSL, no read-back | virtual texturing later
Features (Caves) | CPU circle SDF | GPU carve shader | Rivers/Volcanoes new features
MeshTiler | CPU grid mesh | Marching-Cubes WGSL + GPU buffers | LOD skirts after compute version
Tracing | CPU stdout | Optional GPU timestamp queries | move to ring buffer in WASM memory
Viewer JS | Not started | Read GPU buffers, display via raylib/WebGPU | UI sliders, camera, presets
Server endpoints | Unimplemented | N/A | only static hosting + trace submit
CI & Tests | RNG / IO only | Add shader compilation test via `wgslang` | deterministic mesh checksum

When these items are complete the generator will be fully GPU-accelerated with CPU fallbacks, capable of streaming chunks in real time inside any WebGPU-enabled browser while still running (slowly) in CPU-only environments.

