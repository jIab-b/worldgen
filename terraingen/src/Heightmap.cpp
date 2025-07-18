#include "Heightmap.hpp"
#include "GPUContext.hpp"
#include "Random.hpp"
#ifdef __EMSCRIPTEN__
#include <webgpu/webgpu.h>
#include <emscripten.h>
#include <fstream>
#include <iterator>
#include <string>
#endif

namespace terraingen {

static float RandomFloat01(uint64_t hash) {
    // Convert 24 LSBs to float in range [0,1)
    return static_cast<float>((hash & 0xFFFFFFULL) / double(0x1000000ULL));
}

GPUTexture Heightmap::Generate(const ChunkID& id, GPUContext& gpu) {
    constexpr uint32_t kSize = 256; // 256×256 heightmap for now

    // If we have a WebGPU device, run compute shader path
#ifdef __EMSCRIPTEN__
    if (gpu.HasDevice()) {
        GPUTexture texID = gpu.CreateTexture2D(kSize, kSize);
        auto& texInfo = gpu.GetTexture(texID);

        // --- create shader module (static cache) ---
        static WGPUDevice device = nullptr;
        static WGPUComputePipeline pipeline = nullptr;
        if (!pipeline) {
            device = emscripten_webgpu_get_device();
            // Load noise WGSL from external file
            std::ifstream file("terraingen/shaders/heightmap_noise.wgsl");
            std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (!file) {
                // failed to load shader, fallback to CPU path
            }
            const char* shaderSrc = src.c_str();

            WGPUShaderModuleWGSLDescriptor wgslDesc{};
            wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
            wgslDesc.code = shaderSrc;

            WGPUShaderModuleDescriptor smDesc{};
            smDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
            smDesc.label = "heightmap_noise";

            WGPUShaderModule module = wgpuDeviceCreateShaderModule(device, &smDesc);

            // Pipeline layout inferred
            WGPUComputePipelineDescriptor cpDesc{};
            cpDesc.compute.module = module;
            cpDesc.compute.entryPoint = "main";

            pipeline = wgpuDeviceCreateComputePipeline(device, &cpDesc);
            wgpuShaderModuleRelease(module);
        }

        // --- bind group ---
        WGPUBindGroupLayout bgl = wgpuComputePipelineGetBindGroupLayout(pipeline, 0);

        WGPUBindGroupEntry entry{};
        entry.binding = 0;
        entry.textureView = texInfo.gpuView;

        WGPUBindGroupDescriptor bgDesc{};
        bgDesc.layout = bgl;
        bgDesc.entryCount = 1;
        bgDesc.entries = &entry;

        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);

        // --- command encoder ---
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(device, nullptr);
        WGPUComputePassDescriptor passDesc{};
        WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, &passDesc);
        wgpuComputePassEncoderSetPipeline(pass, pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (kSize + 7)/8, (kSize + 7)/8, 1);
        wgpuComputePassEncoderEnd(pass);

        WGPUCommandBuffer cb = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.Queue(), 1, &cb);
        // Note: we don't map/read back yet; CPU mirror remains zero.

        wgpuBindGroupRelease(bindGroup);
        wgpuCommandBufferRelease(cb);
        wgpuCommandEncoderRelease(enc);

        // --- GPU Erosion Passes ---
        // define cache for both erosion shaders
        struct ErosionCache { WGPUShaderModule module; WGPUBindGroupLayout bgl; WGPUPipelineLayout pl; WGPUComputePipeline pipeline; };
        static ErosionCache hydroCache{};
        static ErosionCache thermoCache{};
        // Hydraulic erosion pass
        GPUTexture hydroTexID = gpu.CreateTexture2D(kSize, kSize);
        auto& hydroTex = gpu.GetTexture(hydroTexID);
        {
            if (!hydroCache.pipeline) {
                std::ifstream efile("terraingen/shaders/hydraulic_erosion.wgsl");
                std::string src((std::istreambuf_iterator<char>(efile)), std::istreambuf_iterator<char>());
                WGPUShaderModuleWGSLDescriptor wgslDesc{}; wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor; wgslDesc.code = src.c_str();
                WGPUShaderModuleDescriptor smDesc{}; smDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
                hydroCache.module = wgpuDeviceCreateShaderModule(device, &smDesc);
                WGPUBindGroupLayoutEntry entries[2]{};
                entries[0].binding = 0; entries[0].visibility = WGPUShaderStage_Compute; entries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat; entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
                entries[1].binding = 1; entries[1].visibility = WGPUShaderStage_Compute; entries[1].storageTexture.access = WGPUStorageTextureAccess_WriteOnly; entries[1].storageTexture.format = WGPUTextureFormat_R32Float; entries[1].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
                WGPUBindGroupLayoutDescriptor bglDesc{}; bglDesc.entryCount = 2; bglDesc.entries = entries;
                hydroCache.bgl = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);
                WGPUPipelineLayoutDescriptor plDesc{}; plDesc.bindGroupLayoutCount = 1; plDesc.bindGroupLayouts = &hydroCache.bgl;
                hydroCache.pl = wgpuDeviceCreatePipelineLayout(device, &plDesc);
                WGPUComputePipelineDescriptor cpDesc{}; cpDesc.layout = hydroCache.pl; cpDesc.compute.module = hydroCache.module; cpDesc.compute.entryPoint = "main";
                hydroCache.pipeline = wgpuDeviceCreateComputePipeline(device, &cpDesc);
            }
            WGPUBindGroupEntry bge[2]{};
            bge[0].binding = 0; bge[0].textureView = texInfo.gpuView;
            bge[1].binding = 1; bge[1].textureView = hydroTex.gpuView;
            WGPUBindGroupDescriptor bgd{}; bgd.layout = hydroCache.bgl; bgd.entryCount = 2; bgd.entries = bge;
            WGPUBindGroup bg = wgpuDeviceCreateBindGroup(device, &bgd);
            WGPUCommandEncoder enc2 = wgpuDeviceCreateCommandEncoder(device, nullptr);
            WGPUComputePassEncoder pass2 = wgpuCommandEncoderBeginComputePass(enc2, nullptr);
            wgpuComputePassEncoderSetPipeline(pass2, hydroCache.pipeline);
            wgpuComputePassEncoderSetBindGroup(pass2, 0, bg, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass2, (kSize + 7)/8, (kSize + 7)/8, 1);
            wgpuComputePassEncoderEnd(pass2);
            WGPUCommandBuffer cb2 = wgpuCommandEncoderFinish(enc2, nullptr);
            wgpuQueueSubmit(gpu.Queue(), 1, &cb2);
            wgpuBindGroupRelease(bg);
            wgpuCommandEncoderRelease(enc2);
            wgpuCommandBufferRelease(cb2);
        }
        // Thermal erosion pass
        GPUTexture thermTexID = gpu.CreateTexture2D(kSize, kSize);
        auto& thermTex = gpu.GetTexture(thermTexID);
        {
            if (!thermoCache.pipeline) {
                std::ifstream efile("terraingen/shaders/thermal_erosion.wgsl");
                std::string src((std::istreambuf_iterator<char>(efile)), std::istreambuf_iterator<char>());
                WGPUShaderModuleWGSLDescriptor wgslDesc{}; wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor; wgslDesc.code = src.c_str();
                WGPUShaderModuleDescriptor smDesc{}; smDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
                thermoCache.module = wgpuDeviceCreateShaderModule(device, &smDesc);
                WGPUBindGroupLayoutEntry entries[2]{};
                entries[0].binding = 0; entries[0].visibility = WGPUShaderStage_Compute; entries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat; entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
                entries[1].binding = 1; entries[1].visibility = WGPUShaderStage_Compute; entries[1].storageTexture.access = WGPUStorageTextureAccess_WriteOnly; entries[1].storageTexture.format = WGPUTextureFormat_R32Float; entries[1].storageTexture.viewDimension = WGPUTextureViewDimension_2D;
                WGPUBindGroupLayoutDescriptor bglDesc{}; bglDesc.entryCount = 2; bglDesc.entries = entries;
                thermoCache.bgl = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);
                WGPUPipelineLayoutDescriptor plDesc{}; plDesc.bindGroupLayoutCount = 1; plDesc.bindGroupLayouts = &thermoCache.bgl;
                thermoCache.pl = wgpuDeviceCreatePipelineLayout(device, &plDesc);
                WGPUComputePipelineDescriptor cpDesc{}; cpDesc.layout = thermoCache.pl; cpDesc.compute.module = thermoCache.module; cpDesc.compute.entryPoint = "main";
                thermoCache.pipeline = wgpuDeviceCreateComputePipeline(device, &cpDesc);
            }
            WGPUBindGroupEntry bge[2]{};
            bge[0].binding = 0; bge[0].textureView = hydroTex.gpuView;
            bge[1].binding = 1; bge[1].textureView = thermTex.gpuView;
            WGPUBindGroupDescriptor bgd{}; bgd.layout = thermoCache.bgl; bgd.entryCount = 2; bgd.entries = bge;
            WGPUBindGroup bg = wgpuDeviceCreateBindGroup(device, &bgd);
            WGPUCommandEncoder enc3 = wgpuDeviceCreateCommandEncoder(device, nullptr);
            WGPUComputePassEncoder pass3 = wgpuCommandEncoderBeginComputePass(enc3, nullptr);
            wgpuComputePassEncoderSetPipeline(pass3, thermoCache.pipeline);
            wgpuComputePassEncoderSetBindGroup(pass3, 0, bg, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass3, (kSize + 7)/8, (kSize + 7)/8, 1);
            wgpuComputePassEncoderEnd(pass3);
            WGPUCommandBuffer cb3 = wgpuCommandEncoderFinish(enc3, nullptr);
            wgpuQueueSubmit(gpu.Queue(), 1, &cb3);
            wgpuBindGroupRelease(bg);
            wgpuCommandEncoderRelease(enc3);
            wgpuCommandBufferRelease(cb3);
        }
        return thermTexID;
    }
#endif

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