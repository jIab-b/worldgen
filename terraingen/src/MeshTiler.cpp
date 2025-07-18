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
                             const GPUTexture sdfTex,
                             GPUContext& gpu) {
    MeshData mesh;
    // GPU path: generate grid mesh on GPU into a vertex buffer
    #ifdef __EMSCRIPTEN__
    if (gpu.HasDevice()) {
        auto device = emscripten_webgpu_get_device();
        const auto& ht = gpu.GetTexture(heightTex);
        uint32_t w = ht.width, h = ht.height;
        // Compile & cache the compute pipeline
        struct PipelineCache { WGPUShaderModule module; WGPUBindGroupLayout bgl; WGPUPipelineLayout pl; WGPUComputePipeline pipeline; };
        static PipelineCache cache{};
        if (!cache.pipeline) {
            std::ifstream file("terraingen/shaders/meshtiler.wgsl");
            std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            WGPUShaderModuleWGSLDescriptor wgslDesc{}; wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor; wgslDesc.code = src.c_str();
            WGPUShaderModuleDescriptor smDesc{}; smDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
            cache.module = wgpuDeviceCreateShaderModule(device, &smDesc);
            // Bind height texture + vertex storage buffer
            WGPUBindGroupLayoutEntry entries[2] = {};
            entries[0].binding = 0; entries[0].visibility = WGPUShaderStage_Compute;
            entries[0].texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
            entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
            entries[1].binding = 1; entries[1].visibility = WGPUShaderStage_Compute;
            entries[1].buffer.type = WGPUBufferBindingType_Storage;
            entries[1].buffer.minBindingSize = static_cast<uint64_t>(w) * h * sizeof(float) * 8;
            WGPUBindGroupLayoutDescriptor bglDesc{}; bglDesc.entryCount = 2; bglDesc.entries = entries;
            cache.bgl = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);
            WGPUPipelineLayoutDescriptor plDesc{}; plDesc.bindGroupLayoutCount = 1; plDesc.bindGroupLayouts = &cache.bgl;
            cache.pl = wgpuDeviceCreatePipelineLayout(device, &plDesc);
            WGPUComputePipelineDescriptor cpDesc{}; cpDesc.layout = cache.pl; cpDesc.compute.module = cache.module; cpDesc.compute.entryPoint = "main";
            cache.pipeline = wgpuDeviceCreateComputePipeline(device, &cpDesc);
        }
        // Allocate GPU buffers
        struct Vertex { float x,y,z,nx,ny,nz,u,v; };
        size_t vertexCount = size_t(w) * h;
        size_t vbufSize = vertexCount * sizeof(Vertex);
        WGPUBufferDescriptor vbDesc{};
        vbDesc.size = vbufSize;
        vbDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc;
        WGPUBuffer vbuf = wgpuDeviceCreateBuffer(device, &vbDesc);
        // Bind group: height texture + vertex buffer
        WGPUBindGroupEntry bgE[2] = {};
        bgE[0].binding = 0; bgE[0].textureView = ht.gpuView;
        bgE[1].binding = 1; bgE[1].buffer = vbuf;
        WGPUBindGroupDescriptor bgDesc{}; bgDesc.layout = cache.bgl; bgDesc.entryCount = 2; bgDesc.entries = bgE;
        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);
        // Dispatch
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(device, nullptr);
        WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cache.pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (w + 7)/8, (h + 7)/8, 1);
        wgpuComputePassEncoderEnd(pass);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.Queue(), 1, &cmd);
        // Read back vertex data
        WGPUBufferDescriptor readDesc{};
        readDesc.size = vbufSize;
        readDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        WGPUBuffer readBuf = wgpuDeviceCreateBuffer(device, &readDesc);
        WGPUCommandEncoder enc2 = wgpuDeviceCreateCommandEncoder(device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc2, vbuf, 0, readBuf, 0, vbufSize);
        WGPUCommandBuffer copyCmd = wgpuCommandEncoderFinish(enc2, nullptr);
        wgpuQueueSubmit(gpu.Queue(), 1, &copyCmd);
        // Map and transfer
        struct UD{bool done;}; UD ud{false};
        wgpuBufferMapAsync(readBuf, WGPUMapMode_Read, 0, vbufSize,
            [](WGPUBufferMapAsyncStatus, void* usr){((UD*)usr)->done = true;}, &ud);
        while(!ud.done) { emscripten_sleep(0); }
        const Vertex* src = (const Vertex*)wgpuBufferGetMappedRange(readBuf, 0, vbufSize);
        mesh.vertices.reserve(vertexCount * 8);
        for(size_t i=0;i<vertexCount;i++){
            mesh.vertices.push_back(src[i].x);
            mesh.vertices.push_back(src[i].y);
            mesh.vertices.push_back(src[i].z);
            mesh.vertices.push_back(src[i].nx);
            mesh.vertices.push_back(src[i].ny);
            mesh.vertices.push_back(src[i].nz);
            mesh.vertices.push_back(src[i].u);
            mesh.vertices.push_back(src[i].v);
        }
        wgpuBufferUnmap(readBuf);
        // Release GPU objects
        wgpuBindGroupRelease(bindGroup);
        wgpuCommandEncoderRelease(enc);
        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(enc2);
        wgpuCommandBufferRelease(copyCmd);
        wgpuBufferRelease(vbuf);
        wgpuBufferRelease(readBuf);
        // CPU fallback for indices
        mesh.indices.reserve((w-1)*(h-1)*6);
        for(uint32_t y=0;y<h-1;++y){
            for(uint32_t x=0;x<w-1;++x){
                uint32_t i0=y*w+x, i1=y*w+(x+1), i2=(y+1)*w+x, i3=(y+1)*w+(x+1);
                mesh.indices.push_back(i0);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i1);
                mesh.indices.push_back(i1);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i3);
            }
        }
        return mesh;
    }
    #endif
    const auto& tex = gpu.GetTexture(heightTex);
    const uint32_t w = tex.width;
    const uint32_t h = tex.height;

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