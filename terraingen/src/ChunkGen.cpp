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
TraceScope::TraceScope(const char* name) {}
TraceScope::~TraceScope() {}

// Random
PCG64State InitPCG64(uint64_t seed, uint64_t seq) {
    PCG64State s; s.state = seed; s.inc = seq; return s;
}
uint64_t PCG64Next(PCG64State& s) { return 0; }
void PCG64Jump(PCG64State& s) {}
uint64_t HashCoords(int64_t x, int64_t y, int64_t z, uint64_t seed) { return seed; }

// Heightmap
GPUTexture Heightmap::Generate(const ChunkID& id, GPUContext& gpu) {
    return 0;
}

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

// IO
std::vector<uint8_t> LoadBinary(const std::string& path) {
    return {};
}
bool SaveBinary(const std::string& path, const std::vector<uint8_t>& data) {
    return true;
}

} // namespace terraingen 