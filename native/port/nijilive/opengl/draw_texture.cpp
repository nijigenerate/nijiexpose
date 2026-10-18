// Port of nijilive/core/render/backends/opengl/draw_texture.d.
#include "draw_texture.hpp"
#include "drawable.hpp"
#include <core/runtime_state.hpp>
#include <core/nodes/part.hpp>
#include <array>
namespace nijiexpose::ogl {
using nicxlive::core::inGetCamera;
extern Shader partShader;
extern GLint gopacity, gMultColor, gScreenColor, mvp, offset;
GLuint quadVertexBuffer;
GLuint quadUvBuffer;
GLuint quadElementBuffer;

void ensureQuadBuffers() {
    if (quadVertexBuffer == 0) {
        glGenBuffers(1, &quadVertexBuffer);
    }
    if (quadUvBuffer == 0) {
        glGenBuffers(1, &quadUvBuffer);
    }
    if (quadElementBuffer == 0) {
        glGenBuffers(1, &quadElementBuffer);
    }
}

void oglDrawTextureAtPart(const Texture* texture, const std::shared_ptr<Part>& part) {
    if (texture == nullptr || part == nullptr) return;

    const float texWidthP = texture->width()/2;
    const float texHeightP = texture->height()/2;

    oglBindDrawableVao();
    ensureQuadBuffers();

    mat4 modelMatrix = part->immediateModelMatrix();
    auto renderSpace = part->currentRenderSpace();

    partShader.use();
    partShader.setUniform(mvp,
        renderSpace.matrix *
        modelMatrix
    );
    partShader.setUniform(offset, part->getMesh().origin);
    partShader.setUniform(gopacity, part->opacity);
    partShader.setUniform(gMultColor, part->tint);
    partShader.setUniform(gScreenColor, part->screenTint);

    oglBindTextureHandle(texture->backendId(), 0);

    constexpr size_t vertexCount = 4;
    std::array<float,8> vertexSoa{-texWidthP, texWidthP, -texWidthP, texWidthP, -texHeightP, -texHeightP, texHeightP, texHeightP};
    std::array<float,8> uvSoa{0,1,0,1,0,0,1,1};
    auto laneBytes = static_cast<ptrdiff_t>(vertexCount) * sizeof(float);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexSoa.size() * sizeof(float), vertexSoa.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(laneBytes));

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, quadUvBuffer);
    glBufferData(GL_ARRAY_BUFFER, uvSoa.size() * sizeof(float), uvSoa.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, quadUvBuffer);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(laneBytes));

    glDisableVertexAttribArray(4);
    glVertexAttrib1f(4, 0);
    glDisableVertexAttribArray(5);
    glVertexAttrib1f(5, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(uint16_t), std::array<uint16_t,6>{0,1,2,2,1,3}.data(), GL_STATIC_DRAW);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
}

void oglDrawTextureAtPosition(const Texture* texture, vec2 position, float opacity, vec3 color, vec3 screenColor) {
    if (texture == nullptr) return;

    vec2 texSize = vec2{float(texture->width()), float(texture->height())};
    vec2 halfSize{texSize.x * 0.5f, texSize.y * 0.5f};
    Rect area{
        position.x - halfSize.x,
        position.y - halfSize.y,
        texSize.x,
        texSize.y
    };

    oglDrawTextureAtRect(texture, area, Rect{0, 0, 1, 1}, opacity, color, screenColor);
}

void oglDrawTextureAtRect(const Texture* texture, Rect area, Rect uvs, float opacity, vec3 color, vec3 screenColor, Shader s, Camera* cam) {
    if (texture == nullptr) return;

    oglBindDrawableVao();
    ensureQuadBuffers();

    if (!s) s = partShader;
    if (!cam) cam = &inGetCamera();
    s.use();
    s.setUniform(s.getUniformLocation("mvp"),
        cam->matrix() *
        mat4::scale(vec3{1,1,1})
    );
    auto offsetLocation = s.getUniformLocation("offset");
    if (offsetLocation != -1) {
        s.setUniform(offsetLocation, vec2{0, 0});
    }
    s.setUniform(s.getUniformLocation("opacity"), opacity);
    s.setUniform(s.getUniformLocation("multColor"), color);
    s.setUniform(s.getUniformLocation("screenColor"), screenColor);

    oglBindTextureHandle(texture->backendId(), 0);

    constexpr size_t vertexCount = 4;
    std::array<float,8> vertexSoa{area.x, area.x+area.w, area.x, area.x+area.w, area.y, area.y, area.y+area.h, area.y+area.h};
    std::array<float,8> uvSoa{uvs.x,uvs.w,uvs.x,uvs.w,uvs.y,uvs.y,uvs.h,uvs.h};
    auto laneBytes = static_cast<ptrdiff_t>(vertexCount) * sizeof(float);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexSoa.size() * sizeof(float), vertexSoa.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(laneBytes));

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, quadUvBuffer);
    glBufferData(GL_ARRAY_BUFFER, uvSoa.size() * sizeof(float), uvSoa.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, quadUvBuffer);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(laneBytes));

    glDisableVertexAttribArray(4);
    glVertexAttrib1f(4, 0);
    glDisableVertexAttribArray(5);
    glVertexAttrib1f(5, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(uint16_t), std::array<uint16_t,6>{0,1,2,2,1,3}.data(), GL_STATIC_DRAW);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
}

}
