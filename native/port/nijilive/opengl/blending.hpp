#pragma once
#include "shader.hpp"
namespace nijiexpose::ogl {
void inInitBlending();
bool inUseMultistageBlending(BlendMode mode);
void nlApplyBlendingCapabilities();
void nlSetBlendingCapabilitiesForTesting(bool advanced, bool coherent, bool fallback);
void nlSetTripleBufferFallback(bool enabled);
bool nlIsTripleBufferFallbackEnabled();
bool inIsAdvancedBlendMode(BlendMode mode);
void inSetBlendMode(BlendMode mode, bool legacyOnly = false);
void inBlendModeBarrier(BlendMode mode);
Shader oglGetBlendShader(BlendMode mode);
void oglBlendToBuffer(Shader shader, BlendMode mode, GLuint dstFramebuffer,
    GLuint bgAlbedo, GLuint bgEmissive, GLuint bgBump,
    GLuint fgAlbedo, GLuint fgEmissive, GLuint fgBump);
}
