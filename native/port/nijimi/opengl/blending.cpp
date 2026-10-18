// nijimi RenderingBackend capability/blend state; host compatibility entry points.
#include "blending.hpp"
#include "part_assets.hpp"
#include <SDL3/SDL.h>
#include <unordered_map>
#include <array>
namespace nijiexpose::ogl {
static bool hasAdvancedBlendSupport() { return SDL_GL_ExtensionSupported("GL_KHR_blend_equation_advanced"); }
static bool hasAdvancedBlendCoherentSupport() { return SDL_GL_ExtensionSupported("GL_KHR_blend_equation_advanced_coherent"); }
static void setAdvancedBlendCoherent(bool e) { oglSetAdvancedBlendCoherent(e); }
static void setLegacyBlendMode(BlendMode m) { oglSetLegacyBlendMode(m); }
static void setAdvancedBlendEquation(BlendMode m) { oglSetAdvancedBlendEquation(m); }
static void issueBlendBarrier() {} // nijimi fallback backend

namespace {
    thread_local bool inAdvancedBlending;
    thread_local bool inAdvancedBlendingCoherent;
    constexpr bool inDefaultTripleBufferFallback = false;
    thread_local bool inForceTripleBufferFallback = inDefaultTripleBufferFallback;
    thread_local bool inAdvancedBlendingAvailable;
    thread_local bool inAdvancedBlendingCoherentAvailable;

    void inApplyBlendingCapabilities() {
        bool desiredAdvanced = inAdvancedBlendingAvailable && !inForceTripleBufferFallback;
        bool desiredCoherent = inAdvancedBlendingCoherentAvailable && !inForceTripleBufferFallback;

        if (desiredCoherent != inAdvancedBlendingCoherent) {
            setAdvancedBlendCoherent(desiredCoherent);
        }

        inAdvancedBlending = desiredAdvanced;
        inAdvancedBlendingCoherent = desiredCoherent;
    }

    void inSetBlendModeLegacy(BlendMode blendingMode) {
        setLegacyBlendMode(blendingMode);
    }
}

/**
    Whether a multi-stage rendering pass should be used for blending
*/
bool inUseMultistageBlending(BlendMode blendingMode) {
    if (inForceTripleBufferFallback) return false;
    switch(blendingMode) {
        case BlendMode::Normal:
        case BlendMode::LinearDodge:
        case BlendMode::AddGlow:
        case BlendMode::Subtract:
        case BlendMode::Inverse:
        case BlendMode::DestinationIn:
        case BlendMode::ClipToLower:
        case BlendMode::SliceFromLower:
                 return false;
        default: return inAdvancedBlending;
    }
}

void nlApplyBlendingCapabilities() {
    inApplyBlendingCapabilities();
}

void nlSetBlendingCapabilitiesForTesting(bool advancedAvailable, bool coherentAvailable, bool forceFallback) {
    inAdvancedBlendingAvailable = advancedAvailable;
    inAdvancedBlendingCoherentAvailable = coherentAvailable;
    inForceTripleBufferFallback = forceFallback;
    inApplyBlendingCapabilities();
}

void inInitBlending() {
    inForceTripleBufferFallback = inDefaultTripleBufferFallback;
    inAdvancedBlendingAvailable = hasAdvancedBlendSupport();
    inAdvancedBlendingCoherentAvailable = hasAdvancedBlendCoherentSupport();
    inApplyBlendingCapabilities();
}

void nlSetTripleBufferFallback(bool enable) {
    if (inForceTripleBufferFallback == enable) return;
    inForceTripleBufferFallback = enable;
    inApplyBlendingCapabilities();
}

bool nlIsTripleBufferFallbackEnabled() {
    return inForceTripleBufferFallback;
}

bool inIsAdvancedBlendMode(BlendMode mode) {
    switch(mode) {
        case BlendMode::Multiply:
        case BlendMode::Screen: 
        case BlendMode::Overlay: 
        case BlendMode::Darken: 
        case BlendMode::Lighten: 
        case BlendMode::ColorDodge: 
        case BlendMode::ColorBurn: 
        case BlendMode::HardLight: 
        case BlendMode::SoftLight: 
        case BlendMode::Difference: 
        case BlendMode::Exclusion: 
            return true;
        
        // Fallback to legacy
        default: 
            return false;
    }
}

void inSetBlendMode(BlendMode blendingMode, bool legacyOnly) {
    if (!inAdvancedBlending || legacyOnly) inSetBlendModeLegacy(blendingMode);
    else setAdvancedBlendEquation(blendingMode);
}

void inBlendModeBarrier(BlendMode mode) {
    if (inAdvancedBlending && !inAdvancedBlendingCoherent && inIsAdvancedBlendMode(mode)) 
        issueBlendBarrier();
}

std::unordered_map<BlendMode, Shader> blendShaders;

void ensureBlendShadersInitialized() {
    if (blendShaders.size() > 0) return;

    auto advancedBlendShader = Shader(AdvancedBlendShaderSource);
    std::array advancedModes{
        BlendMode::Multiply,
        BlendMode::Screen,
        BlendMode::Overlay,
        BlendMode::Darken,
        BlendMode::Lighten,
        BlendMode::ColorDodge,
        BlendMode::ColorBurn,
        BlendMode::HardLight,
        BlendMode::SoftLight,
        BlendMode::Difference,
        BlendMode::Exclusion
    };
    for (auto mode : advancedModes) {
        blendShaders[mode] = advancedBlendShader;
    }
}

Shader oglGetBlendShader(BlendMode mode) {
    ensureBlendShadersInitialized();
    auto shader = blendShaders.find(mode);
    return shader != blendShaders.end() ? shader->second : Shader{};
}

void oglBlendToBuffer(
    Shader shader,
    BlendMode mode,
    GLuint dstFramebuffer,
    GLuint bgAlbedo, GLuint bgEmissive, GLuint bgBump,
    GLuint fgAlbedo, GLuint fgEmissive, GLuint fgBump
) {
    if (!shader) return;

    glBindFramebuffer(GL_FRAMEBUFFER, dstFramebuffer);
    glDrawBuffers(3, std::array<GLenum, 3>{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2}.data());

    shader.use();
    GLint modeUniform = shader.getUniformLocation("blend_mode");
    if (modeUniform != -1) {
        shader.setUniform(modeUniform, static_cast<int>(mode));
    }

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, bgAlbedo);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, bgEmissive);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, bgBump);

    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, fgAlbedo);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, fgEmissive);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, fgBump);

    shader.setUniform(shader.getUniformLocation("bg_albedo"), 0);
    shader.setUniform(shader.getUniformLocation("bg_emissive"), 1);
    shader.setUniform(shader.getUniformLocation("bg_bump"), 2);
    shader.setUniform(shader.getUniformLocation("fg_albedo"), 3);
    shader.setUniform(shader.getUniformLocation("fg_emissive"), 4);
    shader.setUniform(shader.getUniformLocation("fg_bump"), 5);

    GLint mvpUniform = shader.getUniformLocation("mvp");
    if (mvpUniform != -1) {
        shader.setUniform(mvpUniform, mat4::identity());
    }

    GLint offsetUniform = shader.getUniformLocation("offset");
    if (offsetUniform != -1) {
        shader.setUniform(offsetUniform, vec2{0, 0});
    }

    // VAO not used; handled in shader setup.
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
}


}
