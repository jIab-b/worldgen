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
#include <filesystem>
#include <string>

using namespace terraingen;
// -----------------------------------------------------------------------------
// Extracted chunk-generation pipeline for CLI and WASM
// -----------------------------------------------------------------------------
int GenerateChunkCLI(int cx, int cz, const std::string& outDir) {
    // Ensure output directory exists
    std::filesystem::create_directories(outDir);
    ChunkID id{cx, cz};
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

    // 6. Serialize and write outputs
    auto vertBytes = std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(mesh.vertices.data()),
        reinterpret_cast<uint8_t*>(mesh.vertices.data()) + mesh.vertices.size() * sizeof(float)
    );
    auto idxBytes = std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(mesh.indices.data()),
        reinterpret_cast<uint8_t*>(mesh.indices.data()) + mesh.indices.size() * sizeof(uint32_t)
    );

    std::string vPath = outDir + "/chunk_" + std::to_string(cx) + "_" + std::to_string(cz) + "_vertices.bin";
    std::string iPath = outDir + "/chunk_" + std::to_string(cx) + "_" + std::to_string(cz) + "_indices.bin";
    if (!SaveBinary(vPath, vertBytes) || !SaveBinary(iPath, idxBytes)) {
        std::cerr << "Error writing chunk " << cx << "," << cz << " to " << outDir << std::endl;
        return 1;
    }
    std::cout << "Chunk generation complete: " << vPath << " and " << iPath << std::endl;
    // 7. Save heightmap (float32)
    {
        auto& hinfo = gpu.GetTexture(heightTex);
        std::vector<uint8_t> heightBytes(
            reinterpret_cast<uint8_t*>(hinfo.data.data()),
            reinterpret_cast<uint8_t*>(hinfo.data.data()) + hinfo.data.size() * sizeof(float)
        );
        std::string hPath = outDir + "/chunk_" + std::to_string(cx) + "_" + std::to_string(cz) + "_heightmap.raw";
        if (!SaveBinary(hPath, heightBytes)) {
            std::cerr << "Error writing heightmap " << hPath << std::endl;
            return 1;
        }
    }
    // 8. Save biome parameters (uint8_t)
    {
        auto& pinfo = gpu.GetTexture(paramTex);
        const std::vector<uint8_t>& paramBytes = pinfo.dataU8;
        std::string pPath = outDir + "/chunk_" + std::to_string(cx) + "_" + std::to_string(cz) + "_biomeparams.raw";
        if (!SaveBinary(pPath, paramBytes)) {
            std::cerr << "Error writing biome params " << pPath << std::endl;
            return 1;
        }
    }
    // 9. Save SDF (float32) if generated
    if (ctx.sdfTexture != 0) {
        auto& sdfinfo = gpu.GetTexture(ctx.sdfTexture);
        std::vector<uint8_t> sdfBytes(
            reinterpret_cast<uint8_t*>(sdfinfo.data.data()),
            reinterpret_cast<uint8_t*>(sdfinfo.data.data()) + sdfinfo.data.size() * sizeof(float)
        );
        std::string sPath = outDir + "/chunk_" + std::to_string(cx) + "_" + std::to_string(cz) + "_sdf.raw";
        if (!SaveBinary(sPath, sdfBytes)) {
            std::cerr << "Error writing SDF " << sPath << std::endl;
            return 1;
        }
    }
    std::cout << "Heightmap, biome params, and SDF saved to " << outDir << std::endl;
    return 0;
}

// -----------------------------------------------------------------------------
// CLI entrypoint
// -----------------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <cx> <cz> [--outdir <dir>]" << std::endl;
        return 1;
    }
    int cx = std::stoi(argv[1]);
    int cz = std::stoi(argv[2]);
    // Default output directory now points at the viewer's chunks folder
    std::string outDir = "../viewer/chunks";
    if (argc >= 5 && std::string(argv[3]) == "--outdir") {
        outDir = argv[4];
    }
    return GenerateChunkCLI(cx, cz, outDir);
}

// -----------------------------------------------------------------------------
// WASM entrypoint
// -----------------------------------------------------------------------------
extern "C" {
    int GenerateChunk(int cx, int cz) {
        return GenerateChunkCLI(cx, cz, "chunks");
    }
}
// ----------------------------------------------------------------------------- 