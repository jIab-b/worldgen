#include "MeshTiler.hpp"
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

MeshData MeshTiler::Generate(const GPUTexture heightTex,
                             const GPUTexture /*sdfTex*/,
                             GPUContext& gpu) {
    // GPU stub dispatch for mesh tiler
    #ifdef __EMSCRIPTEN__
    if (gpu.HasDevice()) {
        auto device = emscripten_webgpu_get_device();
        const auto& tex = gpu.GetTexture(heightTex);
        uint32_t w = tex.width;
        uint32_t h = tex.height;
        // Compile and cache compute pipeline
        struct PipelineCache { WGPUShaderModule module; WGPUBindGroupLayout bgl; WGPUPipelineLayout pl; WGPUComputePipeline pipeline; };
        static PipelineCache cache{};
        if (!cache.pipeline) {
            std::ifstream file("terraingen/shaders/meshtiler.wgsl");
            std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            WGPUShaderModuleWGSLDescriptor wgslDesc{}; wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor; wgslDesc.code = src.c_str();
            WGPUShaderModuleDescriptor smDesc{}; smDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
            cache.module = wgpuDeviceCreateShaderModule(device, &smDesc);
            WGPUBindGroupLayoutEntry entry{};
            entry.binding = 0;
            entry.visibility = WGPUShaderStage_Compute;
            entry.texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
            entry.texture.viewDimension = WGPUTextureViewDimension_2D;
            WGPUBindGroupLayoutDescriptor bglDesc{}; bglDesc.entryCount = 1; bglDesc.entries = &entry;
            cache.bgl = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);
            WGPUPipelineLayoutDescriptor plDesc{}; plDesc.bindGroupLayoutCount = 1; plDesc.bindGroupLayouts = &cache.bgl;
            cache.pl = wgpuDeviceCreatePipelineLayout(device, &plDesc);
            WGPUComputePipelineDescriptor cpDesc{}; cpDesc.layout = cache.pl; cpDesc.compute.module = cache.module; cpDesc.compute.entryPoint = "main";
            cache.pipeline = wgpuDeviceCreateComputePipeline(device, &cpDesc);
        }
        // Dispatch compute shader
        WGPUBindGroupEntry bgEntry{}; bgEntry.binding = 0; bgEntry.textureView = gpu.GetTexture(heightTex).gpuView;
        WGPUBindGroupDescriptor bgDesc{}; bgDesc.layout = cache.bgl; bgDesc.entryCount = 1; bgDesc.entries = &bgEntry;
        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(device, nullptr);
        WGPUComputePassDescriptor passDesc{};
        WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, &passDesc);
        wgpuComputePassEncoderSetPipeline(pass, cache.pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (w + 7)/8, (h + 7)/8, 1);
        wgpuComputePassEncoderEnd(pass);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.Queue(), 1, &cmd);
        // Cleanup
        wgpuBindGroupRelease(bindGroup);
        wgpuCommandEncoderRelease(enc);
        wgpuCommandBufferRelease(cmd);
    }
    #endif
    const auto& tex = gpu.GetTexture(heightTex);
    const uint32_t w = tex.width;
    const uint32_t h = tex.height;

    MeshData mesh;
    mesh.vertices.reserve(static_cast<size_t>(w) * h * 8); // 8 floats per vert
    mesh.indices.reserve(static_cast<size_t>(w - 1) * (h - 1) * 6);

    auto heightAt = [&](int x, int y) {
        return tex.data[static_cast<size_t>(y) * w + x];
    };

    const float heightScale = 50.0f; // arbitrary vertical scale

    // Generate vertices
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            float hx = static_cast<float>(x);
            float hz = static_cast<float>(y);
            float hy = heightAt(x, y) * heightScale;

            // Position
            mesh.vertices.push_back(hx);
            mesh.vertices.push_back(hy);
            mesh.vertices.push_back(hz);

            // Approximate normal by central differences
            float hL = (x > 0) ? heightAt(x - 1, y) : heightAt(x, y);
            float hR = (x + 1 < w) ? heightAt(x + 1, y) : heightAt(x, y);
            float hD = (y > 0) ? heightAt(x, y - 1) : heightAt(x, y);
            float hU = (y + 1 < h) ? heightAt(x, y + 1) : heightAt(x, y);

            float dx = (hR - hL) * heightScale;
            float dz = (hU - hD) * heightScale;
            // normal = (-dx, 2, -dz) then normalize
            float nx = -dx;
            float ny = 2.0f;
            float nz = -dz;
            float invLen = 1.0f / std::sqrt(nx * nx + ny * ny + nz * nz + 1e-6f);
            nx *= invLen; ny *= invLen; nz *= invLen;
            mesh.vertices.push_back(nx);
            mesh.vertices.push_back(ny);
            mesh.vertices.push_back(nz);

            // UV
            mesh.vertices.push_back(static_cast<float>(x) / (w - 1));
            mesh.vertices.push_back(static_cast<float>(y) / (h - 1));
        }
    }

    // Generate indices
    for (uint32_t y = 0; y < h - 1; ++y) {
        for (uint32_t x = 0; x < w - 1; ++x) {
            uint32_t i0 = y * w + x;
            uint32_t i1 = y * w + (x + 1);
            uint32_t i2 = (y + 1) * w + x;
            uint32_t i3 = (y + 1) * w + (x + 1);
            // Triangle 1
            mesh.indices.push_back(i0);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i1);
            // Triangle 2
            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i3);
        }
    }

    return mesh;
}

} // namespace terraingen 