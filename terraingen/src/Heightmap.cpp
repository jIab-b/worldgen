#include "Heightmap.hpp"
#include "GPUContext.hpp"
#include "Random.hpp"
#ifdef __EMSCRIPTEN__
#include <webgpu/webgpu.h>
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
            const char* shaderSrc = R"(
@group(0) @binding(0) var outputTex : texture_storage_2d<r32float, write>;

@compute @workgroup_size(8,8)
fn main(@builtin(global_invocation_id) gid : vec3<u32>) {
  let dims : vec2<u32> = textureDimensions(outputTex);
  if (gid.x >= dims.x || gid.y >= dims.y) {
    return;
  }
  let x : f32 = f32(gid.x);
  let y : f32 = f32(gid.y);
  let v : f32 = fract(sin((x*12.9898 + y*78.233))*43758.5453);
  textureStore(outputTex, vec2<i32>(gid.xy), vec4<f32>(v, 0.0, 0.0, 0.0));
}
)";

            WGPUShaderModuleWGSLDescriptor wgslDesc{};
            wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
            wgslDesc.code = shaderSrc;

            WGPUShaderModuleDescriptor smDesc{};
            smDesc.nextInChain = &wgslDesc.chain;
            smDesc.label = "heightmap shader";

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

        return texID;
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