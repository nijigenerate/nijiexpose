#include "nijilive/opengl/resources.hpp"
#include "nijilive/opengl/drawable.hpp"
#include "nijilive/opengl/runtime.hpp"
#include <core/runtime_state.hpp>
#include <SDL3/SDL.h>
#include <array>
#include <iostream>
#include <stdexcept>
namespace gl = nijiexpose::ogl;
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
int main() {
    SDL_Window* window{};
    SDL_GLContext context{};
    int result = 0;
    try {
        check(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        window = SDL_CreateWindow("GL resource verification", 32, 32, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        check(window != nullptr, SDL_GetError());
        context = SDL_GL_CreateContext(window);
        check(context != nullptr, SDL_GetError());
        gl::loadApi();
        gl::GLId texture{};
        gl::oglCreateTextureHandle(texture);
        const std::array<uint8_t, 16> pixels{1,2,3,4, 50,60,70,80, 100,110,120,130, 200,210,220,230};
        gl::oglUploadTextureData(texture, 2, 2, 4, 4, false, pixels);
        std::array<uint8_t, 16> readback{};
        gl::oglReadTextureData(texture, 4, false, readback);
        check(readback == pixels, "RGBA upload/readback differs");
        const std::array<uint8_t, 4> replacement{9,8,7,6};
        gl::oglUpdateTextureRegion(texture, 1, 0, 1, 1, 4, replacement);
        gl::oglReadTextureData(texture, 4, false, readback);
        auto expected = pixels;
        std::copy(replacement.begin(), replacement.end(), expected.begin() + 4);
        check(readback == expected, "Texture region update differs");
        for (auto filter : {gl::Filtering::Nearest, gl::Filtering::Linear}) {
            for (bool mipmap : {false, true}) {
                gl::oglApplyTextureFiltering(texture, filter, mipmap);
                GLint value{};
                glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &value);
                const bool linear = filter == gl::Filtering::Linear;
                check(value == (mipmap ? (linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST) : (linear ? GL_LINEAR : GL_NEAREST)), "Minification filter differs");
            }
        }
        for (auto wrap : {gl::Wrapping::Clamp, gl::Wrapping::Repeat, gl::Wrapping::Mirror}) {
            gl::oglApplyTextureWrapping(texture, wrap);
            GLint value{};
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &value);
            check(value == (wrap == gl::Wrapping::Clamp ? GL_CLAMP_TO_BORDER : wrap == gl::Wrapping::Repeat ? GL_REPEAT : GL_MIRRORED_REPEAT), "Wrapping differs");
        }
        gl::oglGenerateTextureMipmap(texture);
        gl::oglBindTextureHandle(texture, 100);
        GLint active{};
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
        check(active == GL_TEXTURE31, "Texture unit clamp differs");
        gl::oglBindTextureHandle(texture, 0);
        gl::ShaderProgramHandle shader;
        gl::oglCreateShaderProgram(shader,
            "#version 330\nuniform mat4 matrix; void main(){gl_Position=matrix*vec4(1,2,3,1);}",
            "#version 330\nout vec4 outputColor; uniform vec4 color; void main(){outputColor=color;}");
        gl::oglUseShaderProgram(shader);
        gl::oglSetUniformMat4(gl::oglShaderGetUniformLocation(shader, "matrix"), gl::mat4::identity());
        gl::oglSetUniformVec4(gl::oglShaderGetUniformLocation(shader, "color"), gl::vec4{1, .5f, .25f, 1});
        gl::oglDestroyShaderProgram(shader);
        check(shader.program == 0 && shader.vert == 0 && shader.frag == 0, "Shader disposal state differs");
        gl::ShaderProgramHandle invalid;
        bool rejected = false;
        try { gl::oglCreateShaderProgram(invalid, "invalid shader", ""); }
        catch (const std::runtime_error&) { rejected = true; }
        check(rejected, "Invalid shader accepted");
        gl::oglDestroyShaderProgram(invalid);
        for (int mode = 0; mode <= 18; ++mode) gl::oglSetLegacyBlendMode(static_cast<gl::BlendMode>(mode));
        gl::oglBindDrawableVao();
        gl::Vec2Array vertices;
        vertices.push_back({-1, -1});
        vertices.push_back({3, -1});
        vertices.push_back({-1, 3});
        gl::oglUploadSharedVertexBuffer(vertices);
        auto readBuffer = reinterpret_cast<PFNGLGETBUFFERSUBDATAPROC>(SDL_GL_GetProcAddress("glGetBufferSubData"));
        check(readBuffer != nullptr, "Missing buffer readback");
        std::array<float, 6> vertexData{};
        readBuffer(GL_ARRAY_BUFFER, 0, sizeof(vertexData), vertexData.data());
        check(vertexData == std::array<float, 6>{-1,3,-1,-1,-1,3}, "SoA vertex upload differs");
        GLuint first{}, second{};
        gl::oglCreateDrawableBuffers(first);
        gl::oglCreateDrawableBuffers(second);
        std::array<uint16_t, 3> triangle{0,1,2};
        gl::oglUploadDrawableIndices(first, triangle);
        // Force shared IBO growth and cached-index re-upload before drawing first.
        std::vector<uint16_t> large(2000, 0);
        gl::oglUploadDrawableIndices(second, large);
        gl::ShaderProgramHandle draw;
        gl::oglCreateShaderProgram(draw,
            "#version 330\nlayout(location=0)in float x;layout(location=1)in float y;void main(){gl_Position=vec4(x,y,0,1);}",
            "#version 330\nout vec4 c;void main(){c=vec4(1,0,0,1);}");
        gl::oglUseShaderProgram(draw);
        gl::glBindBuffer(GL_ARRAY_BUFFER, gl::oglGetSharedVertexBuffer());
        gl::glEnableVertexAttribArray(0);
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
        gl::glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(3 * sizeof(float)));
        glViewport(0, 0, 32, 32);
        glDisable(GL_BLEND);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        gl::oglDrawDrawableElements(first, 3);
        std::array<uint8_t, 4> center{};
        glReadPixels(16, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, center.data());
        check(center == std::array<uint8_t,4>{255,0,0,255}, "Shared-index reallocation changed rendered triangle");
        gl::oglDestroyShaderProgram(draw);
        gl::glDisableVertexAttribArray(0);
        gl::glDisableVertexAttribArray(1);
        check(glGetError() == GL_NO_ERROR, "Drawable GL operation failed");
        gl::oglInitRenderer();
        std::cout << "Init error " << glGetError() << '\n';
        nicxlive::core::inSetViewport(32,32);
        gl::oglResizeViewport(32,32);
        std::cout << "Resize error " << glGetError() << '\n';
        nicxlive::core::inSetClearColor(.25f,.5f,.75f,1.f);
        gl::oglBeginScene();
        std::cout << "Begin error " << glGetError() << '\n';
        std::vector<uint8_t> viewport(32 * 32 * 4);
        gl::oglDumpViewport(viewport,32,32);
        std::cout << "Scene pixel " << int(viewport[0]) << ',' << int(viewport[1]) << ',' << int(viewport[2]) << ',' << int(viewport[3]) << " error " << glGetError() << '\n';
        check(viewport[0] == 64 && (viewport[1] == 127 || viewport[1] == 128) && viewport[2] == 191 && viewport[3] == 255, "Scene clear/readback differs");
        gl::oglAddBasicLightingPostProcess();
        gl::oglPostProcessScene();
        gl::oglDumpViewport(viewport,32,32);
        check(viewport[0] == 64 && (viewport[1] == 127 || viewport[1] == 128) && viewport[2] == 191 && viewport[3] == 255, "Lighting postprocess differs");
        gl::oglDeleteTextureHandle(texture);
        check(texture == 0, "Texture disposal state differs");
        check(glGetError() == GL_NO_ERROR, "OpenGL resource operation failed");
        std::cout << "GL resources passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    if (context) SDL_GL_DestroyContext(context);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
