#pragma once

#include <vector>
#ifdef __EMSCRIPTEN__
#include <webgpu/webgpu.h>
#include <emscripten/html5_webgpu.h>
#else
typedef void* WGPUDevice;
typedef void* WGPUQueue;
typedef void* WGPUTexture;
typedef void* WGPUTextureView;
#endif

namespace terraingen {

// Stub GPU context for compute dispatching (empty for now)
class GPUContext {
public:
    using TextureID = uint32_t;

    struct Texture {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<float> data; // CPU mirror used only for fallback/debug
#ifdef __EMSCRIPTEN__
        WGPUTexture gpuTex = nullptr;
        WGPUTextureView gpuView = nullptr;
#endif
    };

    GPUContext();
    ~GPUContext();

    bool HasDevice() const { return device_ != nullptr; }
#ifdef __EMSCRIPTEN__
    WGPUQueue Queue() const { return queue_; }
#endif

    // Create a 2-D float texture and return its ID
    TextureID CreateTexture2D(uint32_t width, uint32_t height);

    // Access texture by ID (mutable)
    Texture& GetTexture(TextureID id);

    const Texture& GetTexture(TextureID id) const { return textures_.at(id); }

private:
    std::vector<Texture> textures_;

#ifdef __EMSCRIPTEN__
    WGPUDevice device_ = nullptr;
    WGPUQueue queue_ = nullptr;
#endif
};

} // namespace terraingen 