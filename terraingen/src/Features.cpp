#include "Features.hpp"
#include "GPUContext.hpp"
#include "Random.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#ifdef __EMSCRIPTEN__
#include <emscripten/html5_webgpu.h>
#include <emscripten.h>
#include <fstream>
#include <iterator>
#include <string>
#endif

namespace terraingen {

static std::vector<IFeature*>& Registry() {
    static std::vector<IFeature*> vec;
    return vec;
}

void FeatureRegistry::Add(IFeature* feature) {
    Registry().push_back(feature);
}

void FeatureRegistry::ApplyAll(ChunkCtx& ctx) {
    for (auto* f : Registry()) {
        if (f) f->Apply(ctx);
    }
}

// ---------------- Demo Feature: SimpleCaves ----------------
class SimpleCaves : public IFeature {
public:
    void Apply(ChunkCtx& ctx) override {
        // GPU path for cave SDF
#ifdef __EMSCRIPTEN__
        if (ctx.gpu->HasDevice()) {
            // Ensure SDF texture exists
            if (ctx.sdfTexture == 0) {
                const auto& ht = ctx.gpu->GetTexture(ctx.heightTexture);
                ctx.sdfTexture = ctx.gpu->CreateTexture2D(ht.width, ht.height);
            }
            // Compile & cache compute pipeline
            static struct { WGPUShaderModule module; WGPUBindGroupLayout bgl; WGPUPipelineLayout pl; WGPUComputePipeline pipeline; } cache;
            auto device = emscripten_webgpu_get_device();
            if (!cache.pipeline) {
                std::ifstream file("terraingen/shaders/caves.wgsl");
                std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                WGPUShaderModuleWGSLDescriptor wgslDesc{}; wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor; wgslDesc.code = src.c_str();
                WGPUShaderModuleDescriptor smDesc{}; smDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
                cache.module = wgpuDeviceCreateShaderModule(device, &smDesc);
                WGPUBindGroupLayoutEntry entries[2]{};
                entries[0].binding = 0; entries[0].visibility = WGPUShaderStage_Compute; entries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat; entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
                entries[1].binding = 1; entries[1].visibility = WGPUShaderStage_Compute; entries[1].storageTexture.access = WGPUStorageTextureAccess_WriteOnly; entries[1].storageTexture.format = WGPUTextureFormat_R32Float; entries[1].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
                WGPUBindGroupLayoutDescriptor bglDesc{}; bglDesc.entryCount = 2; bglDesc.entries = entries;
                cache.bgl = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);
                WGPUPipelineLayoutDescriptor plDesc{}; plDesc.bindGroupLayoutCount = 1; plDesc.bindGroupLayouts = &cache.bgl;
                cache.pl = wgpuDeviceCreatePipelineLayout(device, &plDesc);
                WGPUComputePipelineDescriptor cpDesc{}; cpDesc.layout = cache.pl; cpDesc.compute.module = cache.module; cpDesc.compute.entryPoint = "main";
                cache.pipeline = wgpuDeviceCreateComputePipeline(device, &cpDesc);
            }
            // Dispatch shader
            const auto& sdf = ctx.gpu->GetTexture(ctx.sdfTexture);
            uint32_t w = sdf.width, h = sdf.height;
            WGPUBindGroupEntry bgEntries[2]{};
            bgEntries[0].binding = 0; bgEntries[0].textureView = ctx.gpu->GetTexture(ctx.heightTexture).gpuView;
            bgEntries[1].binding = 1; bgEntries[1].textureView = sdf.gpuView;
            WGPUBindGroupDescriptor bgd{}; bgd.layout = cache.bgl; bgd.entryCount = 2; bgd.entries = bgEntries;
            WGPUBindGroup bg = wgpuDeviceCreateBindGroup(device, &bgd);
            WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(device, nullptr);
            WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
            wgpuComputePassEncoderSetPipeline(pass, cache.pipeline);
            wgpuComputePassEncoderSetBindGroup(pass, 0, bg, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass, (w+7)/8, (h+7)/8, 1);
            wgpuComputePassEncoderEnd(pass);
            WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
            wgpuQueueSubmit(ctx.gpu->Queue(), 1, &cmd);
            // Cleanup
            wgpuBindGroupRelease(bg);
            wgpuCommandEncoderRelease(enc);
            wgpuCommandBufferRelease(cmd);
            return;
        }
#endif
        // Create SDF texture same resolution as heightmap if not yet
        if (ctx.sdfTexture == 0) {
            const auto& heightTex = ctx.gpu->GetTexture(ctx.heightTexture);
            ctx.sdfTexture = ctx.gpu->CreateTexture2D(heightTex.width, heightTex.height);
            auto& sdf = ctx.gpu->GetTexture(ctx.sdfTexture);
            std::fill(sdf.data.begin(), sdf.data.end(), 1.0f); // positive = empty
        }

        auto& sdf = ctx.gpu->GetTexture(ctx.sdfTexture);
        const auto& hm = ctx.gpu->GetTexture(ctx.heightTexture);

        uint32_t w = hm.width;
        uint32_t h = hm.height;

        // Carve 5 random circular caves based on chunk seed
        uint64_t seed = HashCoords(ctx.id.x, 1234, ctx.id.z, 9876);
        PCG64State rng = InitPCG64(seed);
        auto rand01 = [&](void) {
            return (PCG64Next(rng) >> 40) / double(1ull << 24);
        };

        for (int i = 0; i < 5; ++i) {
            float cx = rand01() * w;
            float cy = rand01() * h;
            float radius = 10.0f + rand01() * 20.0f;
            float r2 = radius * radius;
            for (uint32_t y = 0; y < h; ++y) {
                for (uint32_t x = 0; x < w; ++x) {
                    float dx = x - cx;
                    float dy = y - cy;
                    float dist2 = dx*dx + dy*dy;
                    if (dist2 < r2) {
                        float sdfVal = std::sqrt(dist2) - radius;
                        sdf.data[y * w + x] = std::min(sdf.data[y * w + x], sdfVal);
                    }
                }
            }
        }
    }
};

// Static registration
static SimpleCaves g_simpleCaves;
static bool g_registered = [](){ FeatureRegistry::Add(&g_simpleCaves); return true; }();

} // namespace terraingen 