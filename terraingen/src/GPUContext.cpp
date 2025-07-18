#include "GPUContext.hpp"
#include <utility>

using TextureID = terraingen::GPUContext::TextureID;

namespace terraingen {

GPUContext::GPUContext() {
    // Reserve slot 0 as "null" texture so valid IDs start at 1
    textures_.push_back({});

#ifdef __EMSCRIPTEN__
    device_ = emscripten_webgpu_get_device();
    if (device_) {
        queue_ = wgpuDeviceGetQueue(device_);
    }
#endif
}

GPUContext::~GPUContext() {
#ifdef __EMSCRIPTEN__
    // Cleanup GPU textures
    for (auto& tex : textures_) {
        if (tex.gpuView) wgpuTextureViewRelease(tex.gpuView);
        if (tex.gpuTex) wgpuTextureRelease(tex.gpuTex);
    }
#endif
}

TextureID GPUContext::CreateTexture2D(uint32_t width, uint32_t height) {
    Texture tex;
    tex.width = width;
    tex.height = height;
    tex.data.resize(static_cast<size_t>(width) * height, 0.0f);
#ifdef __EMSCRIPTEN__
    if (device_) {
        WGPUTextureDescriptor td{};
        td.dimension = WGPUTextureDimension_2D;
        td.format = WGPUTextureFormat_R32Float;
        td.size = {width, height, 1};
        td.sampleCount = 1;
        td.mipLevelCount = 1;
        td.usage = WGPUTextureUsage_StorageBinding | WGPUTextureUsage_CopySrc | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        tex.gpuTex = wgpuDeviceCreateTexture(device_, &td);
        WGPUTextureViewDescriptor vd{};
        vd.format = td.format;
        vd.dimension = WGPUTextureViewDimension_2D;
        vd.baseArrayLayer = 0;
        vd.arrayLayerCount = 1;
        vd.baseMipLevel = 0;
        vd.mipLevelCount = 1;
        tex.gpuView = wgpuTextureCreateView(tex.gpuTex, &vd);
    }
#endif
    textures_.push_back(std::move(tex));
    return static_cast<TextureID>(textures_.size() - 1);
}

GPUContext::Texture& GPUContext::GetTexture(TextureID id) {
    return textures_.at(id);
}

} // namespace terraingen 