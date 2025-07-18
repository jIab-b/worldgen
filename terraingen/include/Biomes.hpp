#pragma once

#include <cstdint>
#include <vector>
#include "Heightmap.hpp"

namespace terraingen {

using BiomeMap = std::vector<uint8_t>;

// Biomes classification interface (see implementation.md 4. Biomes.hpp)
class Biomes {
public:
    // Classify height texture into biome IDs
    static BiomeMap Classify(const GPUTexture heightTex, GPUContext& gpu);
    // Generate parameter texture (albedo, roughness, etc.) based on biome map
    static GPUTexture GenerateParameters(const BiomeMap& map, GPUContext& gpu);
};

} // namespace terraingen 