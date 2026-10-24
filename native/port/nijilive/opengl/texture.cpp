// Port of nijilive/core/render/backends/opengl; see project BSD license.
#include "resources.hpp"
#include <array>
#include <limits>
#include <stdexcept>
namespace nijiexpose::ogl {
static GLuint channelFormat(int channels) {
    switch (channels) {
        case 1: return GL_RED;
        case 2: return GL_RG;
        case 3: return GL_RGB;
        default: return GL_RGBA;
    }
}

void oglCreateTextureHandle(GLId& id) {
    GLuint handle;
    glGenTextures(1, &handle);
    if (handle == 0) throw std::runtime_error("Failed to create texture");
    id = handle;
}

void oglDeleteTextureHandle(GLId& id) {
    if (id) {
        GLuint handle = id;
        glDeleteTextures(1, &handle);
        id = 0;
    }
}

void oglBindTextureHandle(GLId id, unsigned unit) {
    glActiveTexture(GL_TEXTURE0 + (unit <= 31 ? unit : 31));
    glBindTexture(GL_TEXTURE_2D, id);
}

void oglUploadTextureData(GLId id, int width, int height, int inChannels, int outChannels, bool stencil, std::span<const uint8_t> data) {
    glBindTexture(GL_TEXTURE_2D, id);
    if (stencil) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        return;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    auto inFormat = channelFormat(inChannels);
    auto outFormat = channelFormat(outChannels);
    glTexImage2D(GL_TEXTURE_2D, 0, outFormat, width, height, 0, inFormat, GL_UNSIGNED_BYTE, data.data());
}

void oglUpdateTextureRegion(GLId id, int x, int y, int width, int height, int channels, std::span<const uint8_t> data) {
    auto format = channelFormat(channels);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, data.data());
}

void oglGenerateTextureMipmap(GLId id) {
    glBindTexture(GL_TEXTURE_2D, id);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void oglApplyTextureFiltering(GLId id, Filtering filtering, bool useMipmaps) {
    glBindTexture(GL_TEXTURE_2D, id);
    bool linear = filtering == Filtering::Linear;
    auto minFilter = useMipmaps
        ? (linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST)
        : (linear ? GL_LINEAR : GL_NEAREST);
    auto magFilter = linear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

void oglApplyTextureWrapping(GLId id, Wrapping wrapping) {
    glBindTexture(GL_TEXTURE_2D, id);
    GLint wrapValue;
    switch (wrapping) {
        case Wrapping::Clamp: wrapValue = GL_CLAMP_TO_BORDER; break;
        case Wrapping::Repeat: wrapValue = GL_REPEAT; break;
        case Wrapping::Mirror: wrapValue = GL_MIRRORED_REPEAT; break;
        default: wrapValue = GL_CLAMP_TO_BORDER; break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapValue);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapValue);
    if (wrapping == Wrapping::Clamp) {
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, std::array<float, 4>{0.f, 0.f, 0.f, 0.f}.data());
    }
}

void oglApplyTextureAnisotropy(GLId id, float value) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, value);
}

float oglMaxTextureAnisotropy() {
    float max = std::numeric_limits<float>::quiet_NaN();
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max);
    return max;
}

void oglReadTextureData(GLId id, int channels, bool stencil, std::span<uint8_t> buffer) {
    glBindTexture(GL_TEXTURE_2D, id);
    GLuint format = stencil ? GL_DEPTH_STENCIL : channelFormat(channels);
    glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, buffer.data());
}

}
