#pragma once

#include <vector>
#include "Heightmap.hpp"
#include "Features.hpp"

namespace terraingen {

// MeshData holds interleaved vertex attributes and indices
struct MeshData {
    std::vector<float> vertices; // interleaved position, normal, uv
    std::vector<uint32_t> indices;
};

// Mesh tiling interface (see implementation.md 4. MeshTiler.hpp)
class MeshTiler {
public:
    // Generate mesh data from height and SDF textures
    static MeshData Generate(const GPUTexture heightTex,
                             const GPUTexture sdfTex,
                             GPUContext& gpu);
};

} // namespace terraingen 