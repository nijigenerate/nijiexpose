// Port of nijilive/core/render/backends/opengl/mask.d.
#include "mask.hpp"
#include "part.hpp"
#include "drawable.hpp"
#include "mask_assets.hpp"
#include <stdexcept>
namespace nijiexpose::ogl {
// D's buffer_sync implementation intentionally disables fences.
static void markBufferInUse(GLuint) {}
Shader maskShader;

GLint maskOffsetUniform;
GLint maskMvpUniform;
bool maskBackendInitialized = false;

void ensureMaskBackendInitialized() {
    if (maskBackendInitialized) return;
    maskBackendInitialized = true;

    maskShader = Shader(MaskShaderSource);
    maskOffsetUniform = maskShader.getUniformLocation("offset");
    maskMvpUniform = maskShader.getUniformLocation("mvp");
}

void oglInitMaskBackend() {
    ensureMaskBackendInitialized();
}

/// Prepare stencil for mask rendering.
/// useStencil == true when there is at least one normal mask (write 1 to masked area).
/// useStencil == false when only dodge masks are present (keep stencil at 1 and punch 0 holes).
void oglBeginMask(bool useStencil) {
    glEnable(GL_STENCIL_TEST);
    // Clear stencil to 0 for normal-mask path, 1 for dodge-only path.
    glClearStencil(useStencil ? 0 : 1);
    glClear(GL_STENCIL_BUFFER_BIT);
    // Reset state to a known baseline before ApplyMask sets specific ops/func.
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, useStencil ? 0 : 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void oglEndMask() {
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glDisable(GL_STENCIL_TEST);
}

void oglBeginMaskContent() {
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilMask(0x00);
}

void oglExecuteMaskPacket(const MaskDrawPacket& packet) {
    ensureMaskBackendInitialized();
    if (packet.indexCount == 0) return;

    oglBindDrawableVao();

    maskShader.use();
    maskShader.setUniform(maskOffsetUniform, packet.origin);
    maskShader.setUniform(maskMvpUniform, packet.mvp);

    if (packet.vertexCount == 0 || packet.vertexAtlasStride == 0 || packet.deformAtlasStride == 0) return;
    auto sharedVbo = oglGetSharedVertexBuffer();
    auto sharedDbo = oglGetSharedDeformBuffer();
    if (sharedVbo == 0 || sharedDbo == 0) return;
    auto vertexOffsetBytes = static_cast<ptrdiff_t>(packet.vertexOffset) * sizeof(float);
    auto vertexStrideBytes = static_cast<ptrdiff_t>(packet.vertexAtlasStride) * sizeof(float);
    auto vertexLane1Offset = vertexStrideBytes + vertexOffsetBytes;
    auto deformOffsetBytes = static_cast<ptrdiff_t>(packet.deformOffset) * sizeof(float);
    auto deformStrideBytes = static_cast<ptrdiff_t>(packet.deformAtlasStride) * sizeof(float);
    auto deformLane1Offset = deformStrideBytes + deformOffsetBytes;

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, sharedVbo);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(vertexOffsetBytes));

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, sharedVbo);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(vertexLane1Offset));

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, sharedDbo);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(deformOffsetBytes));

    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, sharedDbo);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(deformLane1Offset));

    oglDrawDrawableElements(packet.indexBuffer, packet.indexCount);
    markBufferInUse(sharedVbo);
    markBufferInUse(sharedDbo);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
}

void oglExecuteMaskApplyPacket(const MaskApplyPacket& packet) {
    ensureMaskBackendInitialized();
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, packet.isDodge ? 0 : 1, 0xFF);
    glStencilMask(0xFF);

    switch (packet.kind) {
        case MaskDrawableKind::Part:
            oglExecutePartPacket(packet.partPacket);
            break;
        case MaskDrawableKind::Drawable:
            throw std::runtime_error("Unexpected Drawable mask packet; D packet kind must be Part or Mask");
        case MaskDrawableKind::Mask:
            oglExecuteMaskPacket(packet.maskPacket);
            break;
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

}
