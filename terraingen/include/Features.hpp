#pragma once

#include "Heightmap.hpp"
#include "Biomes.hpp"

namespace terraingen {

// Context passed to each feature (holds chunk ID and resource handles)
struct ChunkCtx {
    ChunkID id;
    GPUContext* gpu;
    GPUTexture heightTexture;
    GPUTexture biomeTexture;
    GPUTexture sdfTexture;  // for cave/feature SDFs
};

// Generic feature interface (see implementation.md 4. Features.hpp)
class IFeature {
public:
    virtual ~IFeature() = default;
    // Apply this feature to the chunk context (modify SDF or textures)
    virtual void Apply(ChunkCtx& ctx) = 0;
};

// Registry for dynamic feature modules
class FeatureRegistry {
public:
    static void Add(IFeature* feature);
    static void ApplyAll(ChunkCtx& ctx);
};

} // namespace terraingen 