// Entire nijimi ensurePresentProgram/presentSceneToBackbuffer implementation.
#include "runtime.hpp"
#include "probes.hpp"
#include <array>
namespace nijiexpose::ogl {
static GLuint presentVao{},presentVbo{},presentProgram{};
static GLint presentTexUniform=-1,presentUseColorKeyUniform=-1;
    static void ensurePresentProgram() {
        if (presentProgram != 0 && presentVao != 0 && presentVbo != 0) return;

        const std::string presentVs=R"NIJIGLSL(
#version 330
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUv;
out vec2 texUVs;
void main() {
    texUVs = inUv;
    gl_Position = vec4(inPos, 0.0, 1.0);
})NIJIGLSL";

        const std::string presentFs=R"NIJIGLSL(
#version 330
in vec2 texUVs;
out vec4 outColor;
uniform sampler2D srcTex;
uniform int useColorKey;
void main() {
    vec4 c = texture(srcTex, texUVs);
    if (useColorKey != 0) {
        if (c.a <= 0.001) {
            outColor = vec4(1.0, 0.0, 1.0, 1.0);
        } else {
            vec3 straight = clamp(c.rgb / max(c.a, 0.0001), 0.0, 1.0);
            outColor = vec4(straight, 1.0);
        }
    } else {
        outColor = c;
    }
})NIJIGLSL";

        ShaderProgramHandle handle{};
        oglCreateShaderProgram(handle,presentVs,presentFs);
        presentProgram=handle.program;
        glDeleteShader(handle.vert);
        glDeleteShader(handle.frag);

        presentTexUniform = glGetUniformLocation(presentProgram, "srcTex");
        presentUseColorKeyUniform = glGetUniformLocation(presentProgram, "useColorKey");

        glGenVertexArrays(1, &presentVao);
        glGenBuffers(1, &presentVbo);
        glBindVertexArray(presentVao);
        glBindBuffer(GL_ARRAY_BUFFER, presentVbo);
        const std::array<float,24> quad = {
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
        };
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(quad.size() * sizeof(float)), quad.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void oglPresentSceneToBackbuffer(int width, int height, bool useColorKeyTransparency) {
        ensurePresentProgram();

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK);
        glViewport(0, 0, width, height);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        if (useColorKeyTransparency) {
            glClearColor(1, 0, 1, 1);
        } else {
            glClearColor(0, 0, 0, 0);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(presentProgram);
        if (presentTexUniform != -1) glUniform1i(presentTexUniform, 0);
        if (presentUseColorKeyUniform != -1) glUniform1i(presentUseColorKeyUniform, useColorKeyTransparency ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, oglGetMainAlbedo());
        glBindVertexArray(presentVao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        oglCaptureFinalAlphaProbe(width,height);
        glUseProgram(0);
        glFlush();
    }


}
