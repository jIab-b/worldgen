#pragma once

#include <cstdint>
#include "Random.hpp"

namespace terraingen {

// Identifier for a chunk in the world grid
typedef struct { int x, z; } ChunkID;

// Forward declare GPU context and texture handle
class GPUContext;
using GPUTexture = uint32_t;

// Heightmap generation interface (see implementation.md 4. Heightmap.hpp)
class Heightmap {
public:
    // Dispatches FBM noise + erosion compute passes; returns a GPU texture handle
    static GPUTexture Generate(const ChunkID& id, GPUContext& gpu);
};

} // namespace terraingen 