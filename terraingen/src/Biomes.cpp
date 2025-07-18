#include "Biomes.hpp"
#include "GPUContext.hpp"
#include <cmath>
#ifdef __EMSCRIPTEN__
#include <webgpu/webgpu.h>
#include <emscripten.h>
#include <fstream>
#include <iterator>
#include <string>
#endif

namespace terraingen {

BiomeMap Biomes::Classify(const GPUTexture heightTex, GPUContext& gpu) {
    const auto& heightInfo = gpu.GetTexture(heightTex);
    uint32_t w = heightInfo.width;
    uint32_t h = heightInfo.height;

#ifdef __EMSCRIPTEN__
    if (gpu.HasDevice()) {
        // 1. Create output R8 texture
        GPUTexture outTexID = gpu.CreateTexture2D_U8(w, h);
        auto& outTex = gpu.GetTexture(outTexID);

        WGPUDevice device = emscripten_webgpu_get_device();

        // --- create / cache shader module & pipeline ---
        struct PipelineCache {
            WGPUShaderModule module = nullptr;
            WGPUBindGroupLayout bgl = nullptr;
            WGPUPipelineLayout pl = nullptr;
            WGPUComputePipeline pipeline = nullptr;
        };
        static PipelineCache cache;
        if (!cache.pipeline) {
            // Load WGSL shader source from external file
            std::ifstream file("terraingen/shaders/biome_classify.wgsl");
            std::string wgslSrc((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (!file) {
                // Failed to load shader file; fallback to CPU path
            }
            WGPUShaderModuleWGSLDescriptor wgslDesc{};
            wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
            wgslDesc.code = wgslSrc.c_str();
            WGPUShaderModuleDescriptor smDesc{};
            smDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
            cache.module = wgpuDeviceCreateShaderModule(device, &smDesc);

            // Bind group layout (uniform, height tex, writable tex)
            WGPUBindGroupLayoutEntry entries[3] = {};
            entries[0].binding = 0;
            entries[0].visibility = WGPUShaderStage_Compute;
            entries[0].buffer.type = WGPUBufferBindingType_Uniform;
            entries[0].buffer.minBindingSize = sizeof(float) * 2;

            entries[1].binding = 1;
            entries[1].visibility = WGPUShaderStage_Compute;
            entries[1].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
            entries[1].texture.viewDimension = WGPUTextureViewDimension_2D;

            entries[2].binding = 2;
            entries[2].visibility = WGPUShaderStage_Compute;
            entries[2].storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
            entries[2].storageTexture.format = WGPUTextureFormat_R8Uint;
            entries[2].storageTexture.viewDimension = WGPUTextureViewDimension_2D;

            WGPUBindGroupLayoutDescriptor bglDesc{};
            bglDesc.entryCount = 3;
            bglDesc.entries = entries;
            cache.bgl = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);

            WGPUPipelineLayoutDescriptor plDesc{};
            plDesc.bindGroupLayoutCount = 1;
            plDesc.bindGroupLayouts = &cache.bgl;
            cache.pl = wgpuDeviceCreatePipelineLayout(device, &plDesc);

            WGPUComputePipelineDescriptor cpDesc{};
            cpDesc.layout = cache.pl;
            cpDesc.compute.module = cache.module;
            cpDesc.compute.entryPoint = "main";
            cache.pipeline = wgpuDeviceCreateComputePipeline(device, &cpDesc);
        }

        // --- Create uniform buffer with thresholds ---
        struct ThresholdUBO { float low; float mid; } thresholds{0.33f, 0.66f};
        WGPUBufferDescriptor ubDesc{};
        ubDesc.size = sizeof(ThresholdUBO);
        ubDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        ubDesc.mappedAtCreation = false;
        WGPUBuffer ubo = wgpuDeviceCreateBuffer(device, &ubDesc);
        wgpuQueueWriteBuffer(gpu.Queue(), ubo, 0, &thresholds, sizeof(thresholds));

        // Bind group
        WGPUBindGroupEntry bgEntries[3] = {};
        bgEntries[0].binding = 0;
        bgEntries[0].buffer = ubo;
        bgEntries[0].offset = 0;
        bgEntries[0].size = sizeof(ThresholdUBO);

        bgEntries[1].binding = 1;
        bgEntries[1].textureView = heightInfo.gpuView;

        bgEntries[2].binding = 2;
        bgEntries[2].textureView = outTex.gpuView;

        WGPUBindGroupDescriptor bgDesc{};
        bgDesc.layout = cache.bgl;
        bgDesc.entryCount = 3;
        bgDesc.entries = bgEntries;
        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);

        // Command encoder & pass
        WGPUCommandEncoderDescriptor encDesc{};
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encDesc);

        WGPUComputePassDescriptor passDesc{};
        WGPUComputePassEncoder cpass = wgpuCommandEncoderBeginComputePass(encoder, &passDesc);
        wgpuComputePassEncoderSetPipeline(cpass, cache.pipeline);
        wgpuComputePassEncoderSetBindGroup(cpass, 0, bindGroup, 0, nullptr);
        uint32_t wgX = (w + 7) / 8;
        uint32_t wgY = (h + 7) / 8;
        wgpuComputePassEncoderDispatchWorkgroups(cpass, wgX, wgY, 1);
        wgpuComputePassEncoderEnd(cpass);

        WGPUCommandBufferDescriptor cbDesc{};
        WGPUCommandBuffer cmdBuf = wgpuCommandEncoderFinish(encoder, &cbDesc);
        wgpuQueueSubmit(gpu.Queue(), 1, &cmdBuf);

        // --- Read back outputTex into CPU ---
        size_t byteSize = static_cast<size_t>(w) * h;
        WGPUBufferDescriptor readDesc{};
        readDesc.size = byteSize;
        readDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        readDesc.mappedAtCreation = false;
        WGPUBuffer readBuf = wgpuDeviceCreateBuffer(device, &readDesc);

        WGPUImageCopyTexture srcTex{};
        srcTex.texture = outTex.gpuTex;
        srcTex.mipLevel = 0;
        srcTex.origin = {0,0,0};

        WGPUImageCopyBuffer dstBuf{};
        dstBuf.buffer = readBuf;
        dstBuf.layout.bytesPerRow = w;
        dstBuf.layout.rowsPerImage = h;

        WGPUExtent3D copySize = {w, h, 1};
        WGPUCommandEncoder encoder2 = wgpuDeviceCreateCommandEncoder(device, nullptr);
        wgpuCommandEncoderCopyTextureToBuffer(encoder2, &srcTex, &dstBuf, &copySize);
        WGPUCommandBuffer copyCmd = wgpuCommandEncoderFinish(encoder2, nullptr);
        wgpuQueueSubmit(gpu.Queue(), 1, &copyCmd);

        // Wait and map
        struct UserData { bool done; } userdata{false};
        auto callback = [](WGPUBufferMapAsyncStatus status, void* userdata) {
            auto* ud = static_cast<UserData*>(userdata);
            ud->done = true;
        };
        wgpuBufferMapAsync(readBuf, WGPUMapMode_Read, 0, byteSize, callback, &userdata);
        // busy wait (not ideal but fine for WASM single-thread demo)
        while(!userdata.done) { emscripten_sleep(0); }

        const uint8_t* mapped = static_cast<const uint8_t*>(wgpuBufferGetMappedRange(readBuf, 0, byteSize));
        BiomeMap gpuMap(byteSize);
        memcpy(gpuMap.data(), mapped, byteSize);
        wgpuBufferUnmap(readBuf);

        // Move data into outTex CPU mirror for debug
        outTex.dataU8.assign(gpuMap.begin(), gpuMap.end());

        return gpuMap;
    }
#endif

    // CPU fallback / data for pipeline
    size_t pixelCount = static_cast<size_t>(w) * h;
    BiomeMap map(pixelCount);
    for (size_t i = 0; i < pixelCount; ++i) {
        float v = heightInfo.data[i];
        uint8_t id = (v < 0.33f) ? 0 : (v < 0.66f ? 1 : 2);
        map[i] = id;
    }
    return map;
}

GPUTexture Biomes::GenerateParameters(const BiomeMap& map, GPUContext& gpu) {
    if (map.empty()) return 0;
    uint32_t size = static_cast<uint32_t>(std::sqrt(map.size()));
    GPUTexture texID = gpu.CreateTexture2D(size, size);
    auto& tex = gpu.GetTexture(texID);
    for (size_t i = 0; i < map.size(); ++i) {
        uint8_t id = map[i];
        tex.data[i] = id == 0 ? 0.2f : (id == 1 ? 0.6f : 0.8f);
    }
    return texID;
}

} // namespace terraingen 