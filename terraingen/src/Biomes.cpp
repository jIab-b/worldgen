#include "Biomes.hpp"
#include "GPUContext.hpp"
#include <cmath>

namespace terraingen {

BiomeMap Biomes::Classify(const GPUTexture heightTex, GPUContext& gpu) {
    const auto& tex = gpu.GetTexture(heightTex);
    size_t pixelCount = static_cast<size_t>(tex.width) * tex.height;
    BiomeMap map(pixelCount);

    for (size_t i = 0; i < pixelCount; ++i) {
        float h = tex.data[i];
        uint8_t id = 0;
        if (h < 0.33f)
            id = 0; // water / beach
        else if (h < 0.66f)
            id = 1; // plains / grassland
        else
            id = 2; // mountain / snow
        map[i] = id;
    }
    return map;
}

GPUTexture Biomes::GenerateParameters(const BiomeMap& map, GPUContext& gpu) {
    // Create single-channel parameter texture: albedo brightness per biome id
    if (map.empty()) return 0;

    // We need width/height; assume same 256x256 as heightmap (square)
    uint32_t size = static_cast<uint32_t>(std::sqrt(map.size()));
    GPUTexture texID = gpu.CreateTexture2D(size, size);
    auto& tex = gpu.GetTexture(texID);

    for (size_t i = 0; i < map.size(); ++i) {
        uint8_t id = map[i];
        float value = 0.0f;
        switch (id) {
            case 0: value = 0.2f; break; // dark (water)
            case 1: value = 0.6f; break; // medium (grass)
            case 2: value = 0.8f; break; // light (snow)
            default: value = 0.5f; break;
        }
        tex.data[i] = value;
    }
    return texID;
}

} // namespace terraingen 