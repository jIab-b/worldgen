#pragma once

#include <vector>

namespace terraingen {

// Stub GPU context for compute dispatching (empty for now)
class GPUContext {
public:
    using TextureID = uint32_t;

    struct Texture {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<float> data; // single-channel R32F for now
    };

    GPUContext();
    ~GPUContext();

    // Create a 2-D float texture and return its ID
    TextureID CreateTexture2D(uint32_t width, uint32_t height);

    // Access texture by ID (mutable)
    Texture& GetTexture(TextureID id);

    const Texture& GetTexture(TextureID id) const { return textures_.at(id); }

private:
    std::vector<Texture> textures_;
};

} // namespace terraingen 