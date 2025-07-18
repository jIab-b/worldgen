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

// Biomes implementation moved to Biomes.cpp

// Features
void FeatureRegistry::Add(IFeature* feature) {}
void FeatureRegistry::ApplyAll(ChunkCtx& ctx) {}

// TextureSynth implementation moved to TextureSynth.cpp

// MeshTiler implementation moved to MeshTiler.cpp

// IO implementations moved to IO.cpp
// std::vector<uint8_t> LoadBinary(...)
// bool SaveBinary(...)

} // namespace terraingen 