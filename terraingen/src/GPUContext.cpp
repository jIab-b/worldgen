#include "GPUContext.hpp"
#include <utility>

using TextureID = terraingen::GPUContext::TextureID;

namespace terraingen {

GPUContext::GPUContext() {
    // Reserve slot 0 as "null" texture so valid IDs start at 1
    textures_.push_back({});
}

GPUContext::~GPUContext() = default;

TextureID GPUContext::CreateTexture2D(uint32_t width, uint32_t height) {
    Texture tex;
    tex.width = width;
    tex.height = height;
    tex.data.resize(static_cast<size_t>(width) * height, 0.0f);
    textures_.push_back(std::move(tex));
    return static_cast<TextureID>(textures_.size() - 1);
}

GPUContext::Texture& GPUContext::GetTexture(TextureID id) {
    return textures_.at(id);
}

} // namespace terraingen 