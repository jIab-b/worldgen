#include "Tracing.hpp"
#include "Random.hpp"
#include "Heightmap.hpp"
#include "Biomes.hpp"
#include "Features.hpp"
#include "TextureSynth.hpp"
#include "MeshTiler.hpp"
#include "IO.hpp"
#include "GPUContext.hpp"
#include <iostream>
#include <vector>
#include <cstdint>

using namespace terraingen;

int main(int argc, char** argv) {
    // For now, use fixed ChunkID and out-dir
    ChunkID id{0, 0};
    GPUContext gpu;

    // Trace generation
    TraceScope trace("GenerateChunk");

    // 1. Heightmap
    GPUTexture heightTex = Heightmap::Generate(id, gpu);

    // 2. Biomes
    BiomeMap biomeMap = Biomes::Classify(heightTex, gpu);
    GPUTexture paramTex = Biomes::GenerateParameters(biomeMap, gpu);

    // 3. Features
    ChunkCtx ctx{id, &gpu, heightTex, paramTex, 0};
    FeatureRegistry::ApplyAll(ctx);

    // 4. Textures
    GPUTexture albedo, normal, roughness;
    TextureSynth::Generate(heightTex, biomeMap, gpu, albedo, normal, roughness);

    // 5. Mesh
    MeshData mesh = MeshTiler::Generate(heightTex, ctx.sdfTexture, gpu);

    // 6. Write outputs
    // Serialize vertices (float) and indices (uint32_t)
    auto vertBytes = std::vector<uint8_t>(
        (uint8_t*)mesh.vertices.data(),
        (uint8_t*)mesh.vertices.data() + mesh.vertices.size() * sizeof(float)
    );
    auto idxBytes = std::vector<uint8_t>(
        (uint8_t*)mesh.indices.data(),
        (uint8_t*)mesh.indices.data() + mesh.indices.size() * sizeof(uint32_t)
    );

    SaveBinary("chunk_0_0_vertices.bin", vertBytes);
    SaveBinary("chunk_0_0_indices.bin",  idxBytes);

    std::cout << "Chunk generation complete. Outputs written." << std::endl;
    return 0;
}

extern "C" {
    // Exported entrypoint for WebAssembly (matches EXPORTED_FUNCTIONS)
    void GenerateChunk() {
        // Invoke the same pipeline as main
        main(0, nullptr);
    }
} 