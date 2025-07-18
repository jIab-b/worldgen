#include "TextureSynth.hpp"
#include "GPUContext.hpp"
#include <cmath>

namespace terraingen {

void TextureSynth::Generate(const GPUTexture /*heightTex*/,
                             const BiomeMap& biomeMap,
                             GPUContext& gpu,
                             GPUTexture& outAlbedo,
                             GPUTexture& outNormal,
                             GPUTexture& outRoughness) {
    if (biomeMap.empty()) {
        outAlbedo = outNormal = outRoughness = 0;
        return;
    }

    uint32_t size = static_cast<uint32_t>(std::sqrt(biomeMap.size()));
    auto createAndFill = [&](GPUTexture& texID, float (*mapFn)(uint8_t)) {
        texID = gpu.CreateTexture2D(size, size);
        auto& tex = gpu.GetTexture(texID);
        for (size_t i = 0; i < biomeMap.size(); ++i) {
            tex.data[i] = mapFn(biomeMap[i]);
        }
    };

    // Albedo brightness
    createAndFill(outAlbedo, [](uint8_t id) {
        switch (id) {
            case 0: return 0.2f; // water dark
            case 1: return 0.5f; // grass mid
            case 2: return 0.9f; // snow bright
            default: return 0.5f;
        }
    });

    // Normal Y component only (encoded as brightness 1.0)
    createAndFill(outNormal, [](uint8_t) { return 1.0f; });

    // Roughness value per biome
    createAndFill(outRoughness, [](uint8_t id) {
        switch (id) {
            case 0: return 0.4f; // water smoother
            case 1: return 0.9f; // grass rougher (diffuse)
            case 2: return 0.3f; // snow semi-smooth
            default: return 0.5f;
        }
    });
}

} // namespace terraingen 