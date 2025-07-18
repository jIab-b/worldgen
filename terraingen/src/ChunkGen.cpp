#pragma once or appropriate

#include "Tracing.hpp"
#include "Random.hpp"
#include "Heightmap.hpp"
#include "Biomes.hpp"
#include "Features.hpp"
#include "TextureSynth.hpp"
#include "MeshTiler.hpp"
#include "IO.hpp"
#include "GPUContext.hpp"

namespace terraingen {

// TraceScope implementation
// NOTE: TraceScope implementation moved to Tracing.cpp
// TraceScope::TraceScope(const char* name) {}
// TraceScope::~TraceScope() {}

// Random implementation moved to Random.cpp
// PCG64State InitPCG64(uint64_t seed, uint64_t seq) {...}
// uint64_t PCG64Next(PCG64State& s) {...}
// void PCG64Jump(PCG64State& s) {...}
// uint64_t HashCoords(...) {...}

// Biomes
BiomeMap Biomes::Classify(const GPUTexture heightTex, GPUContext& gpu) {
    return {};
}
GPUTexture Biomes::GenerateParameters(const BiomeMap& map, GPUContext& gpu) {
    return 0;
}

// Features
void FeatureRegistry::Add(IFeature* feature) {}
void FeatureRegistry::ApplyAll(ChunkCtx& ctx) {}

// TextureSynth
void TextureSynth::Generate(const GPUTexture heightTex,
                             const BiomeMap& biomeMap,
                             GPUContext& gpu,
                             GPUTexture& outAlbedo,
                             GPUTexture& outNormal,
                             GPUTexture& outRoughness) {
    outAlbedo = outNormal = outRoughness = 0;
}

// MeshTiler
MeshData MeshTiler::Generate(const GPUTexture heightTex,
                              const GPUTexture sdfTex,
                              GPUContext& gpu) {
    return {};
}

// IO implementations moved to IO.cpp
// std::vector<uint8_t> LoadBinary(...)
// bool SaveBinary(...)

} // namespace terraingen 