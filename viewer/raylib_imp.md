# Raylib Viewer Integration Plan

This document outlines a complete plan to integrate the procedural terrain generator output into the existing Raylib-based viewer (`viewer/raylib_viewer.c`).  A new developer can follow these steps to achieve a full, interactive 3D demo in the browser or desktop.

---

## 1. Prerequisites

- Raylib (web port or desktop) installed
- Emscripten SDK (for web build)
- CMake or Make (for CLI generator)
- C++17 toolchain

---

## 2. Data Outputs from Generator

1. **Vertices**: `chunk_<cx>_<cz>_vertices.bin` (float32 interleaved: x,y,z,nx,ny,nz,u,v)
2. **Indices**:  `chunk_<cx>_<cz>_indices.bin`  (uint32 triangle list)
3. (Optional) **Textures**: `chunk_<cx>_<cz>_<map>.raw` (float32 or uint8 arrays for height, biome, albedo, normal, roughness)

We will target the mesh loader path; textures can be added in Phase 2.

---

## 3. Viewer Code Refactor

File: `viewer/raylib_viewer.c`

### 3.1 Remove Old Voxel Loader

- Delete `blocks`/`colors` globals, `loadChunk()`, and `drawChunk()`.

### 3.2 Add Mesh Data Globals

```c
static float*    verts     = NULL;
static uint32_t* inds      = NULL;
static int       vCount    = 0;
static int       iCount    = 0;
static Mesh      mesh      = { 0 };
```

### 3.3 Implement `loadChunkMesh(int cx, int cz)`

```c
static void loadChunkMesh(int cx, int cz) {
    // Free previous data
    if (verts) UnloadFileData((unsigned char*)verts);
    if (inds ) UnloadFileData((unsigned char*)inds );

    // Load vertex blob
    size_t sizeV = 0;
    unsigned char* blobV = LoadFileData(TextFormat("chunks/chunk_%d_%d_vertices.bin",cx,cz), &sizeV);
    vCount = sizeV / sizeof(float);
    verts  = (float*)blobV;

    // Load index blob
    size_t sizeI = 0;
    unsigned char* blobI = LoadFileData(TextFormat("chunks/chunk_%d_%d_indices.bin",cx,cz), &sizeI);
    iCount = sizeI / sizeof(uint32_t);
    inds   = (uint32_t*)blobI;

    // Build Raylib Mesh
    mesh.vertexCount   = vCount/8;
    mesh.triangleCount = iCount/3;
    // set pointers into interleaved array (stride=sizeof(float)*8)
    mesh.vertices      = verts;
    mesh.normals       = verts + 3;
    mesh.texcoords     = verts + 6;
    mesh.indices       = (unsigned short*)inds; // or cast/convert if >65535
    UploadMesh(&mesh, false);
}
```

### 3.4 Replace `main()` Logic

- In `main()`, after initializing camera and window:
  ```c
    loadChunkMesh(0, 0);
  ```
- In the main draw loop, replace `drawChunk()`:
  ```c
    BeginMode3D(camera);
      DrawMesh(mesh, MatrixIdentity(), WHITE);
      DrawGrid(10, 1.0f);
    EndMode3D();
  ```

### 3.5 Add Reload & UI Controls

- On keypress (e.g. `R`), call `loadChunkMesh(newCX,newCZ);`
- Display FPS, chunk coords, and camera info with `DrawText()`.

---

## 4. Build & Packaging

### 4.1 Embed Chunk Files

In `viewer/compile.py`, ensure:
```python
# embed all chunk binaries
cmd += ["--embed-file", "chunks"]
```
This makes `chunks/` available in the WASM virtual FS.

### 4.2 Web Build Command

```bash
cd viewer
python3 compile.py raylib_viewer.c -o mesh_viewer.html
```

### 4.3 Desktop Build (optional)

- Link against native Raylib, remove Emscripten flags.

---

## 5. Phase 2 Extensions

1. **Texture Mapping**: load `albedo.raw`, `normal.raw`, `roughness.raw` into `Texture2D` and apply in `DrawMeshEx()` / custom shader.
2. **Regeneration UI**: Toggle between CPU/GPU or change presets on the fly via GUI.
3. **Trace Export**: Hook browser `fetch()` to POST Chrome trace JSON from GPU passes.
4. **LOD & Streaming**: Dynamically load neighboring chunks.

---

## 6. Testing & Validation

- Verify geometry matches external exporters (Blender import of `.bin`).
- Confirm camera controls behave as expected.
- Test fallback for missing chunk files (show error UI).

---

End of Raylib Viewer Implementation Plan 