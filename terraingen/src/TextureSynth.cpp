#include "TextureSynth.hpp"
#include "GPUContext.hpp"
#include <cmath>
#ifdef __EMSCRIPTEN__
#include <emscripten/html5_webgpu.h>
#include <emscripten.h>
#include <fstream>
#include <iterator>
#include <string>
#endif

namespace terraingen {

void TextureSynth::Generate(const GPUTexture heightTex,
                             const BiomeMap& biomeMap,
                             GPUContext& gpu,
                             GPUTexture& outAlbedo,
                             GPUTexture& outNormal,
                             GPUTexture& outRoughness) {
    if (biomeMap.empty()) {
        outAlbedo = outNormal = outRoughness = 0;
        return;
    }

    // GPU path: load and dispatch texturesynth shader
#ifdef __EMSCRIPTEN__
    if (gpu.HasDevice()) {
        auto device = emscripten_webgpu_get_device();
        uint32_t size = static_cast<uint32_t>(std::sqrt(biomeMap.size()));
        struct PipelineCache { WGPUShaderModule module; WGPUBindGroupLayout bgl; WGPUPipelineLayout pl; WGPUComputePipeline pipeline; };
        static PipelineCache cache{};
        if (!cache.pipeline) {
            std::ifstream file("terraingen/shaders/texturesynth.wgsl");
            std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            WGPUShaderModuleWGSLDescriptor wgslDesc{}; wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor; wgslDesc.code = src.c_str();
            WGPUShaderModuleDescriptor smDesc{}; smDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
            cache.module = wgpuDeviceCreateShaderModule(device, &smDesc);
            WGPUBindGroupLayoutEntry entries[4]{};
            entries[0].binding = 0; entries[0].visibility = WGPUShaderStage_Compute; entries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat; entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
            entries[1].binding = 1; entries[1].visibility = WGPUShaderStage_Compute; entries[1].storageTexture.access = WGPUStorageTextureAccess_WriteOnly; entries[1].storageTexture.format = WGPUTextureFormat_R32Float; entries[1].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
            entries[2].binding = 2; entries[2].visibility = WGPUShaderStage_Compute; entries[2].storageTexture.access = WGPUStorageTextureAccess_WriteOnly; entries[2].storageTexture.format = WGPUTextureFormat_R32Float; entries[2].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
            entries[3].binding = 3; entries[3].visibility = WGPUShaderStage_Compute; entries[3].storageTexture.access = WGPUStorageTextureAccess_WriteOnly; entries[3].storageTexture.format = WGPUTextureFormat_R32Float; entries[3].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
            WGPUBindGroupLayoutDescriptor bglDesc{}; bglDesc.entryCount = 4; bglDesc.entries = entries;
            cache.bgl = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);
            WGPUPipelineLayoutDescriptor plDesc{}; plDesc.bindGroupLayoutCount = 1; plDesc.bindGroupLayouts = &cache.bgl;
            cache.pl = wgpuDeviceCreatePipelineLayout(device, &plDesc);
            WGPUComputePipelineDescriptor cpDesc{}; cpDesc.layout = cache.pl; cpDesc.compute.module = cache.module; cpDesc.compute.entryPoint = "main";
            cache.pipeline = wgpuDeviceCreateComputePipeline(device, &cpDesc);
        }
        // allocate outputs
        outAlbedo   = gpu.CreateTexture2D(size, size);
        outNormal   = gpu.CreateTexture2D(size, size);
        outRoughness= gpu.CreateTexture2D(size, size);
        auto& aTex = gpu.GetTexture(outAlbedo);
        auto& nTex = gpu.GetTexture(outNormal);
        auto& rTex = gpu.GetTexture(outRoughness);
        // bind group
        WGPUBindGroupEntry bgEntries[4]{};
        bgEntries[0].binding = 0; bgEntries[0].textureView = gpu.GetTexture(heightTex).gpuView;
        bgEntries[1].binding = 1; bgEntries[1].textureView = aTex.gpuView;
        bgEntries[2].binding = 2; bgEntries[2].textureView = nTex.gpuView;
        bgEntries[3].binding = 3; bgEntries[3].textureView = rTex.gpuView;
        WGPUBindGroupDescriptor bgDesc{}; bgDesc.layout = cache.bgl; bgDesc.entryCount = 4; bgDesc.entries = bgEntries;
        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);
        // dispatch workgroups
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(device, nullptr);
        WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cache.pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
        uint32_t wgX = (size + 7) / 8;
        uint32_t wgY = (size + 7) / 8;
        wgpuComputePassEncoderDispatchWorkgroups(pass, wgX, wgY, 1);
        wgpuComputePassEncoderEnd(pass);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.Queue(), 1, &cmd);
        // cleanup
        wgpuBindGroupRelease(bindGroup);
        wgpuCommandEncoderRelease(enc);
        wgpuCommandBufferRelease(cmd);
        return;
    }
#endif

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