#pragma once

#include <vector>
#include "Heightmap.hpp"
#include "Biomes.hpp"

namespace terraingen {

// Texture synthesis interface (see implementation.md 4. TextureSynth.hpp)
class TextureSynth {
public:
    // Generate albedo, normal, and roughness textures from height and biome inputs
    static void Generate(const GPUTexture heightTex,
                         const BiomeMap& biomeMap,
                         GPUContext& gpu,
                         GPUTexture& outAlbedo,
                         GPUTexture& outNormal,
                         GPUTexture& outRoughness);
};

} // namespace terraingen 