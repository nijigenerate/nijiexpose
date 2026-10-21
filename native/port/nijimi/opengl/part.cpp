// Executed nijimi RenderingBackend part implementation, translated to C++ packet types.
// Shader stage selection translated from nijimi
// source/render/opengl/opengl_backend.d::RenderingBackend.setupShaderStage.
#include "part.hpp"
#include "blending.hpp"
#include "runtime.hpp"
#include "drawable.hpp"
#include "part_assets.hpp"
#include <core/nodes/part.hpp>
#include <core/runtime_state.hpp>
#include <algorithm>
namespace nijiexpose::ogl {
using std::min;
using nicxlive::core::inGetViewport;
static void setupShaderStage(const PartDrawPacket&, int, mat4, mat4);
static void renderStage(const PartDrawPacket&, bool);
// The D buffer_sync module deliberately disables fences and does no work here.
static void markBufferInUse(GLuint) {}

    Shader partShader;
    Shader partShaderStage1;
    Shader partShaderStage2;
    Shader partMaskShader;
    GLint mvp;
    GLint offset;
    GLint gopacity;
    GLint gMultColor;
    GLint gScreenColor;
    GLint gEmissionStrength;
    GLint gs1mvp;
    GLint gs1offset;
    GLint gs1opacity;
    GLint gs1MultColor;
    GLint gs1ScreenColor;
    GLint gs2mvp;
    GLint gs2offset;
    GLint gs2opacity;
    GLint gs2EmissionStrength;
    GLint gs2MultColor;
    GLint gs2ScreenColor;
    GLint mmvp;
    GLint mthreshold;
bool partBackendInitialized = false;






void oglInitPartBackendResources() {
        if (partBackendInitialized) return;
        partBackendInitialized = true;

        partShader = Shader(PartShaderSource);
        partShaderStage1 = Shader(PartShaderStage1Source);
        partShaderStage2 = Shader(PartShaderStage2Source);
        partMaskShader = Shader(PartMaskShaderSource);

        oglBindDrawableVao();

        partShader.use();
        partShader.setUniform(partShader.getUniformLocation("albedo"), 0);
        partShader.setUniform(partShader.getUniformLocation("emissive"), 1);
        partShader.setUniform(partShader.getUniformLocation("bumpmap"), 2);
        mvp = partShader.getUniformLocation("mvp");
        offset = partShader.getUniformLocation("offset");
        gopacity = partShader.getUniformLocation("opacity");
        gMultColor = partShader.getUniformLocation("multColor");
        gScreenColor = partShader.getUniformLocation("screenColor");
        gEmissionStrength = partShader.getUniformLocation("emissionStrength");

        partShaderStage1.use();
        partShaderStage1.setUniform(partShaderStage1.getUniformLocation("albedo"), 0);
        gs1mvp = partShaderStage1.getUniformLocation("mvp");
        gs1offset = partShaderStage1.getUniformLocation("offset");
        gs1opacity = partShaderStage1.getUniformLocation("opacity");
        gs1MultColor = partShaderStage1.getUniformLocation("multColor");
        gs1ScreenColor = partShaderStage1.getUniformLocation("screenColor");

        partShaderStage2.use();
        partShaderStage2.setUniform(partShaderStage2.getUniformLocation("emissive"), 1);
        partShaderStage2.setUniform(partShaderStage2.getUniformLocation("bumpmap"), 2);
        gs2mvp = partShaderStage2.getUniformLocation("mvp");
        gs2offset = partShaderStage2.getUniformLocation("offset");
        gs2opacity = partShaderStage2.getUniformLocation("opacity");
        gs2MultColor = partShaderStage2.getUniformLocation("multColor");
        gs2ScreenColor = partShaderStage2.getUniformLocation("screenColor");
        gs2EmissionStrength = partShaderStage2.getUniformLocation("emissionStrength");

        partMaskShader.use();
        partMaskShader.setUniform(partMaskShader.getUniformLocation("albedo"), 0);
        partMaskShader.setUniform(partMaskShader.getUniformLocation("emissive"), 1);
        partMaskShader.setUniform(partMaskShader.getUniformLocation("bumpmap"), 2);
        mmvp = partMaskShader.getUniformLocation("mvp");
        mthreshold = partMaskShader.getUniformLocation("threshold");
    }

void oglDrawPartPacket(const PartDrawPacket& packet) {
    if (!packet.renderable) return;
    oglExecutePartPacket(packet);
}

void oglExecutePartPacket(const PartDrawPacket& packet) {
        auto node=packet.node.lock(); if(!node)return; auto textures=node->textures;
        if (textures.size() == 0) return;

        oglBindDrawableVao();

        if (boundAlbedo != textures[0]) {
            for(size_t i=0;i<textures.size();++i) {
                auto& tex=textures[i]; if(tex) {
                    oglBindTextureHandle(tex->backendId(), static_cast<unsigned>(i));
                } else {
                    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned>(i));
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
            boundAlbedo = textures[0];
        }

        auto matrix = packet.modelMatrix;
        mat4 renderMatrix = packet.renderMatrix;

        if (packet.isMask) {
            mat4 mvpMatrix = renderMatrix * matrix;

            partMaskShader.use();
            partMaskShader.setUniform(offset, packet.origin);
            partMaskShader.setUniform(mmvp, mvpMatrix);
            partMaskShader.setUniform(mthreshold, packet.maskThreshold);

            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            renderStage(packet, false);
        } else {
            if (inUseMultistageBlending(packet.blendMode)) {
                setupShaderStage(packet, 0, matrix, renderMatrix);
                renderStage(packet, true);

                if (packet.hasEmissionOrBumpmap) {
                    setupShaderStage(packet, 1, matrix, renderMatrix);
                    renderStage(packet, false);
                }
            } else {
                setupShaderStage(packet, 2, matrix, renderMatrix);
                renderStage(packet, false);
            }
        }

        glDrawBuffers(3, std::array<GLenum, 3>{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2}.data());
        glBlendEquation(GL_FUNC_ADD);
    }

void setupShaderStage(const PartDrawPacket& packet, int stage, mat4 matrix, mat4 renderMatrix) {
        mat4 mvpMatrix = renderMatrix * matrix;

        auto setDrawBuffersSafe = [&](int desired) {
            std::array<GLenum,3> bufs{};
            int count = 0;
            auto addIfPresent = [&](GLenum att) {
                GLint type = GL_NONE;
                glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, att,
                    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
                if (type != GL_NONE) {
                    bufs[count++] = att;
                }
            };
            addIfPresent(GL_COLOR_ATTACHMENT0);
            if (desired > 1) addIfPresent(GL_COLOR_ATTACHMENT1);
            if (desired > 2) addIfPresent(GL_COLOR_ATTACHMENT2);
            if (count == 0) {
                glDrawBuffer(GL_BACK);
            } else {
                glDrawBuffers(count, bufs.data());
            }
            return count;
        };

        switch (stage) {
            case 0:
                setDrawBuffersSafe(1);
                partShaderStage1.use();
                partShaderStage1.setUniform(gs1offset, packet.origin);
                partShaderStage1.setUniform(gs1mvp, mvpMatrix);
                partShaderStage1.setUniform(gs1opacity, packet.opacity);
                partShaderStage1.setUniform(gs1MultColor, packet.clampedTint);
                partShaderStage1.setUniform(gs1ScreenColor, packet.clampedScreen);
                inSetBlendMode(packet.blendMode, false);
                break;
            case 1:
                setDrawBuffersSafe(2);
                partShaderStage2.use();
                partShaderStage2.setUniform(gs2offset, packet.origin);
                partShaderStage2.setUniform(gs2mvp, mvpMatrix);
                partShaderStage2.setUniform(gs2opacity, packet.opacity);
                partShaderStage2.setUniform(gs2EmissionStrength, packet.emissionStrength);
                partShaderStage2.setUniform(gs2MultColor, packet.clampedTint);
                partShaderStage2.setUniform(gs2ScreenColor, packet.clampedScreen);
                inSetBlendMode(packet.blendMode, true);
                break;
            case 2:
                setDrawBuffersSafe(3);
                partShader.use();
                partShader.setUniform(offset, packet.origin);
                partShader.setUniform(mvp, mvpMatrix);
                partShader.setUniform(gopacity, packet.opacity);
                partShader.setUniform(gEmissionStrength, packet.emissionStrength);
                partShader.setUniform(gMultColor, packet.clampedTint);
                partShader.setUniform(gScreenColor, packet.clampedScreen);
                inSetBlendMode(packet.blendMode, true);
                break;
            default:
                return;
        }
    }

void renderStage(const PartDrawPacket& packet, bool advanced) {
        auto ibo = static_cast<GLuint>(packet.indexBuffer);
        auto indexCount = packet.indexCount;

        if (!ibo || indexCount == 0 || packet.vertexCount == 0) return;
        if (packet.vertexAtlasStride == 0 || packet.uvAtlasStride == 0 || packet.deformAtlasStride == 0) return;

        auto vertexBuffer = oglGetSharedVertexBuffer();
        auto uvBuffer = oglGetSharedUvBuffer();
        auto deformBuffer = oglGetSharedDeformBuffer();
        if (vertexBuffer == 0 || uvBuffer == 0 || deformBuffer == 0) return;

        auto vertexOffsetBytes = static_cast<ptrdiff_t>(packet.vertexOffset) * sizeof(float);
        auto uvOffsetBytes = static_cast<ptrdiff_t>(packet.uvOffset) * sizeof(float);
        auto deformOffsetBytes = static_cast<ptrdiff_t>(packet.deformOffset) * sizeof(float);

        auto vertexStrideBytes = static_cast<ptrdiff_t>(packet.vertexAtlasStride) * sizeof(float);
        auto uvStrideBytes = static_cast<ptrdiff_t>(packet.uvAtlasStride) * sizeof(float);
        auto deformStrideBytes = static_cast<ptrdiff_t>(packet.deformAtlasStride) * sizeof(float);

        auto vertexLane1Offset = vertexStrideBytes + vertexOffsetBytes;
        auto uvLane1Offset = uvStrideBytes + uvOffsetBytes;
        auto deformLane1Offset = deformStrideBytes + deformOffsetBytes;
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(vertexOffsetBytes));

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(vertexLane1Offset));

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(uvOffsetBytes));

        glEnableVertexAttribArray(3);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(uvLane1Offset));

        glEnableVertexAttribArray(4);
        glBindBuffer(GL_ARRAY_BUFFER, deformBuffer);
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(deformOffsetBytes));

        glEnableVertexAttribArray(5);
        glBindBuffer(GL_ARRAY_BUFFER, deformBuffer);
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(deformLane1Offset));

        oglDrawDrawableElements(packet.indexBuffer, indexCount);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
        glDisableVertexAttribArray(4);
        glDisableVertexAttribArray(5);

        if (advanced) {
            inBlendModeBarrier(packet.blendMode);
        }
    }

}
