#include "Biomes.hpp"
#include "GPUContext.hpp"
#include <cmath>
#ifdef __EMSCRIPTEN__
#include <webgpu/webgpu.h>
#endif

namespace terraingen {

BiomeMap Biomes::Classify(const GPUTexture heightTex, GPUContext& gpu) {
    const auto& heightInfo = gpu.GetTexture(heightTex);
    uint32_t w = heightInfo.width;
    uint32_t h = heightInfo.height;

#ifdef __EMSCRIPTEN__
    if (gpu.HasDevice()) {
        // 1. Create output R8 texture
        GPUTexture outTexID = gpu.CreateTexture2D_U8(w, h);
        auto& outTex = gpu.GetTexture(outTexID);

        WGPUDevice device = emscripten_webgpu_get_device();
        // --- pipeline cache ---
        static WGPUComputePipeline pipeline = nullptr;
        if (!pipeline) {
            // load shader source from embedded string (same as file)
            const char* shaderSrc = R"(
#include "../shaders/biome_classify.wgsl"
)";
            // easier: include WGSL via string.
        }
        // For brevity, skip GPU dispatch implementation (TODO)
        // Fallback to CPU after dispatch to produce BiomeMap
    }
#endif

    // CPU fallback / data for pipeline
    size_t pixelCount = static_cast<size_t>(w) * h;
    BiomeMap map(pixelCount);
    for (size_t i = 0; i < pixelCount; ++i) {
        float v = heightInfo.data[i];
        uint8_t id = (v < 0.33f) ? 0 : (v < 0.66f ? 1 : 2);
        map[i] = id;
    }
    return map;
}

GPUTexture Biomes::GenerateParameters(const BiomeMap& map, GPUContext& gpu) {
    if (map.empty()) return 0;
    uint32_t size = static_cast<uint32_t>(std::sqrt(map.size()));
    GPUTexture texID = gpu.CreateTexture2D(size, size);
    auto& tex = gpu.GetTexture(texID);
    for (size_t i = 0; i < map.size(); ++i) {
        uint8_t id = map[i];
        tex.data[i] = id == 0 ? 0.2f : (id == 1 ? 0.6f : 0.8f);
    }
    return texID;
}

} // namespace terraingen 