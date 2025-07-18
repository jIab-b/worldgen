## Additions Since Initial Checkpoint

### New CPU Implementations
8. **MeshTiler.cpp** – builds a full grid mesh from the height-map (positions, normals, UVs) and fills vertex/index buffers; results are now written to `chunk_0_0_vertices.bin` / `indices.bin`.
9. **TextureSynth.cpp** – generates placeholder albedo/normal/roughness textures per biome.
10. **Features.cpp** – basic `FeatureRegistry` plus a sample `SimpleCaves` feature that carves circular caves into a 2-D SDF texture.

Generated mesh and textures are still CPU-only but now contain non-empty data suitable for a viewer.

---

## Upcoming Focus – GPU Acceleration Roadmap

The codebase is still CPU-bound; the next milestones shift heavy stages to WebGPU.

1. **GPUContext overhaul**
   • Manage real `WGPUDevice`, queue, and texture/buffer handles.  
   • Add `-s USE_WEBGPU=1` to `compile.py`.
2. **Height-map compute shader (`heightmap.wgsl`)**
   • 4-octave FBM noise; dispatch over 512×512.  
   • Read-back optional (temporary, for biome CPU fallback).
3. **Biome classification shader**
   • Generates R8Uint biome ID texture directly on GPU; CPU no longer loops over pixels.
4. **Texture synthesis shader (`triplanar.wgsl`)**
   • Produces albedo/normal/roughness RGBA8 textures; stays on GPU.
5. **Cave SDF + Marching Cubes shaders**
   • GPU builds 3-D SDF then triangulates via compute; mesh buffers remain GPU-resident and are passed to viewer.
6. **Viewer integration**
   • JS fetches GPU buffers via WebGPU, constructs raylib Mesh with indirect draw.

---

## Updated Build & Test Checklist
- [x] CPU generation pipeline produces populated height, biome, mesh, cave SDF, and material textures.  
- [ ] `GPUContext` switched to WebGPU device abstraction.  
- [ ] `heightmap.wgsl` compiled and dispatches successfully.  
- [ ] End-to-end chunk generation time ≤5 ms on desktop GPU.  
- [ ] Viewer renders the GPU-generated mesh without CPU read-back.

---
