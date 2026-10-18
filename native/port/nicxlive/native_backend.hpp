#pragma once
#include <core/render/common.hpp>
namespace nicxlive::core {
// Additional D renderer operations missing from nicxlive's queue-oriented API.
class NativeBackend {
public:
    virtual ~NativeBackend() = default;
    virtual uint32_t createNativeTexture(const std::vector<uint8_t>&, int, int, int, int, bool, bool) = 0;
    virtual void setNativeTextureFiltering(uint32_t, Filtering, bool) = 0;
    virtual void setNativeTextureWrapping(uint32_t, Wrapping) = 0;
    virtual void setNativeTextureAnisotropy(uint32_t, float) = 0;
    virtual void setNativeTextureData(uint32_t, const std::vector<uint8_t>&, int) = 0;
    virtual std::vector<uint8_t> readNativeTexture(uint32_t) = 0;
    virtual bool useNativeMultistageBlend(nodes::BlendMode) = 0;
    virtual std::unique_ptr<RenderCommandEmitter> createNativeCommandEmitter() = 0;
};
}
