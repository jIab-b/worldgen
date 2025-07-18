#include "Heightmap.hpp"
#include "GPUContext.hpp"
#include "Random.hpp"

namespace terraingen {

static float RandomFloat01(uint64_t hash) {
    // Convert 24 LSBs to float in range [0,1)
    return static_cast<float>((hash & 0xFFFFFFULL) / double(0x1000000ULL));
}

GPUTexture Heightmap::Generate(const ChunkID& id, GPUContext& gpu) {
    constexpr uint32_t kSize = 256; // 256×256 heightmap for now

    GPUTexture texID = gpu.CreateTexture2D(kSize, kSize);
    auto& tex = gpu.GetTexture(texID);

    // Simple 4-octave FBM using HashCoords as value noise
    constexpr int kOctaves = 4;
    constexpr float lacunarity = 2.0f;
    constexpr float gain = 0.5f;

    for (uint32_t y = 0; y < kSize; ++y) {
        for (uint32_t x = 0; x < kSize; ++x) {
            float amp = 1.0f;
            float freq = 1.0f / 64.0f; // base frequency
            float value = 0.0f;
            for (int o = 0; o < kOctaves; ++o) {
                int64_t sampleX = static_cast<int64_t>(id.x * kSize + x) * freq * 1000.0f;
                int64_t sampleZ = static_cast<int64_t>(id.z * kSize + y) * freq * 1000.0f;
                uint64_t h = HashCoords(sampleX, 0, sampleZ, 1337u + static_cast<uint64_t>(o));
                float n = RandomFloat01(h) * 2.0f - 1.0f; // [-1,1]
                value += n * amp;
                amp *= gain;
                freq *= lacunarity;
            }
            // Normalize to [0,1]
            value = value * 0.5f + 0.5f;
            tex.data[y * kSize + x] = value;
        }
    }

    return texID;
}

} // namespace terraingen 