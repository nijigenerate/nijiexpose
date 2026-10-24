// Complete translation of nijimi/source/render/opengl/opengl_thumb.d.
#pragma once
#include "api.hpp"
#include <array>
#include <span>
#include <string>
#include <stdexcept>
namespace nijiexpose::ogl {
class DebugTextureBackend {
private:
    GLuint debugThumbTex{};
    GLuint debugThumbProg{};
    GLint debugThumbMvpLoc = -1;
    GLuint debugThumbVao{};
    GLuint debugThumbQuadVbo{};
    GLuint debugThumbQuadEbo{};

    void ensureThumbVao() {
        if (debugThumbVao == 0) {
            glGenVertexArrays(1, &debugThumbVao);
        }
        if (debugThumbQuadVbo == 0) glGenBuffers(1, &debugThumbQuadVbo);
        if (debugThumbQuadEbo == 0) glGenBuffers(1, &debugThumbQuadEbo);
    }

    void ensureThumbProgram() {
        if (debugThumbProg != 0) return;
        const std::string vsSrc=R"NIJIGLSL(
            #version 330 core
            uniform mat4 mvp;
            layout(location = 0) in vec2 inPos;
            layout(location = 1) in vec2 inUv;
            out vec2 vUv;
            void main() {
                gl_Position = mvp * vec4(inPos, 0.0, 1.0);
                vUv = inUv;
            }
        )NIJIGLSL";
        const std::string fsSrc=R"NIJIGLSL(
            #version 330 core
            in vec2 vUv;
            layout(location = 0) out vec4 outColor;
            uniform sampler2D albedo;
            void main() {
                outColor = texture(albedo, vUv);
            }
        )NIJIGLSL";
        auto compile = [](GLenum kind, const std::string& src) {
            GLuint s = glCreateShader(kind);
            const char* p = src.c_str();
            glShaderSource(s, 1, &p, nullptr);
            glCompileShader(s);
            GLint ok = 0;
            glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if(ok != GL_TRUE) throw std::runtime_error("thumb shader compile failed");
            return s;
        };
        GLuint vs = compile(GL_VERTEX_SHADER, vsSrc);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc);
        debugThumbProg = glCreateProgram();
        glAttachShader(debugThumbProg, vs);
        glAttachShader(debugThumbProg, fs);
        glLinkProgram(debugThumbProg);
        GLint linked = 0;
        glGetProgramiv(debugThumbProg, GL_LINK_STATUS, &linked);
        if(linked != GL_TRUE) throw std::runtime_error("thumb shader link failed");
        glDeleteShader(vs);
        glDeleteShader(fs);
        glUseProgram(debugThumbProg);
        GLint albedoLoc = glGetUniformLocation(debugThumbProg, "albedo");
        if (albedoLoc >= 0) glUniform1i(albedoLoc, 0);
        debugThumbMvpLoc = glGetUniformLocation(debugThumbProg, "mvp");
        glUseProgram(0);
    }

public:
    void ensureDebugTestTex() {
        if (debugThumbTex != 0) return;
        const int sz = 48;
        std::array<uint8_t,sz*sz*4> pixels{};
        for(int y=0;y<sz;++y) for(int x=0;x<sz;++x) {
            bool on = ((x / 6) ^ (y / 6)) & 1;
            auto idx = (y * sz + x) * 4;
            pixels[idx + 0] = on ? 255 : 30;
            pixels[idx + 1] = on ? 128 : 30;
            pixels[idx + 2] = on ? 64 : 30;
            pixels[idx + 3] = 255;
        }
        glGenTextures(1, &debugThumbTex);
        glBindTexture(GL_TEXTURE_2D, debugThumbTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sz, sz, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    void drawTile(GLuint texId, float x, float y, float size, int screenW, int screenH) {
        if (texId == 0) return;
        ensureThumbProgram();
        ensureThumbVao();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screenW, screenH);
        glUseProgram(debugThumbProg);
        float left = (x / static_cast<float>(screenW)) * 2.0f - 1.0f;
        float right = ((x + size) / static_cast<float>(screenW)) * 2.0f - 1.0f;
        float top = (y / static_cast<float>(screenH)) * 2.0f - 1.0f;
        float bottom = ((y + size) / static_cast<float>(screenH)) * 2.0f - 1.0f;
        if (debugThumbMvpLoc >= 0) {
            std::array<float,16> ident = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            glUniformMatrix4fv(debugThumbMvpLoc, 1, GL_FALSE, ident.data());
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);
        std::array<float,24> verts = {
            left, top, 0, 0,
            right, top, 1, 0,
            left, bottom, 0, 1,
            right, top, 1, 0,
            right, bottom, 1, 1,
            left, bottom, 0, 1
        };
        glBindVertexArray(debugThumbVao);
        glBindBuffer(GL_ARRAY_BUFFER, debugThumbQuadVbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, static_cast<int>(4 * sizeof(float)), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, static_cast<int>(4 * sizeof(float)), reinterpret_cast<void*>(2 * sizeof(float)));
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
        glDisableVertexAttribArray(4);
        glDisableVertexAttribArray(5);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void renderThumbnailGrid(int screenW, int screenH, std::span<const GLuint> textureIds) {
        GLint prevFbo = 0, prevProgram = 0, prevVao = 0, prevDrawBuf = 0;
        GLboolean prevDepth = 0, prevStencil = 0, prevCull = 0, prevScissor = 0;
        std::array<GLint,4> prevViewport{};
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevFbo);
        glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
        glGetIntegerv(GL_DRAW_BUFFER, &prevDrawBuf);
        glGetIntegerv(GL_VIEWPORT, prevViewport.data());
        prevDepth = glIsEnabled(GL_DEPTH_TEST);
        prevStencil = glIsEnabled(GL_STENCIL_TEST);
        prevCull = glIsEnabled(GL_CULL_FACE);
        prevScissor = glIsEnabled(GL_SCISSOR_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glViewport(0, 0, screenW, screenH);
        glDrawBuffer(GL_BACK);

        glEnable(GL_SCISSOR_TEST);
        const float tile = 48;
        const float pad = 2;
        float sidebarWidthPx = (tile + pad) * 8;
        glScissor(0, 0, static_cast<int>(sidebarWidthPx), screenH);
        glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);

        ensureDebugTestTex();
        float tx = pad;
        float ty = pad;
        drawTile(debugTestTextureId(), tx, ty, tile, screenW, screenH);
        ty += tile + pad;
        for(auto texId:textureIds) {
            drawTile(texId, tx, ty, tile, screenW, screenH);
            ty += tile + pad;
            if (ty + tile > screenH - pad) {
                ty = pad;
                tx += tile + pad;
            }
        }
        glGetError();

        if (prevDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (prevStencil) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
        if (prevCull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (prevScissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
        glDrawBuffer(prevDrawBuf);
        glUseProgram(prevProgram);
        glBindVertexArray(prevVao);
    }

    GLuint debugTestTextureId() const {
        return debugThumbTex;
    }
};
inline DebugTextureBackend& currentDebugTextureBackend(){ static DebugTextureBackend instance; return instance; }
}
