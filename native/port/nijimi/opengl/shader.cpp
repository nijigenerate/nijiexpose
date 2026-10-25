// nijimi RenderingBackend shader creation, uniforms and lifetime.
#include "resources.hpp"
#include <array>
#include <limits>
#include <stdexcept>
namespace nijiexpose::ogl {
static void checkShader(GLuint shader) {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            std::string log(length, '\0');
            glGetShaderInfoLog(shader, length, nullptr, log.data());
            throw std::runtime_error(log);
        }
        throw std::runtime_error("Shader compile failed");
    }
}

static void checkProgram(GLuint program) {
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            std::string log(length, '\0');
            glGetProgramInfoLog(program, length, nullptr, log.data());
            throw std::runtime_error(log);
        }
        throw std::runtime_error("Program link failed");
    }
}

void oglCreateShaderProgram(ShaderProgramHandle& handle, const std::string& vertex, const std::string& fragment) {
    handle.vert = glCreateShader(GL_VERTEX_SHADER);
    auto vsrc = vertex.c_str();
    glShaderSource(handle.vert, 1, &vsrc, nullptr);
    glCompileShader(handle.vert);
    checkShader(handle.vert);

    handle.frag = glCreateShader(GL_FRAGMENT_SHADER);
    auto fsrc = fragment.c_str();
    glShaderSource(handle.frag, 1, &fsrc, nullptr);
    glCompileShader(handle.frag);
    checkShader(handle.frag);

    handle.program = glCreateProgram();
    glAttachShader(handle.program, handle.vert);
    glAttachShader(handle.program, handle.frag);
    glLinkProgram(handle.program);
    checkProgram(handle.program);
}

void oglDestroyShaderProgram(ShaderProgramHandle& handle) {
    if (handle.program) {
        glDetachShader(handle.program, handle.vert);
        glDetachShader(handle.program, handle.frag);
        glDeleteProgram(handle.program);
    }
    if (handle.vert) glDeleteShader(handle.vert);
    if (handle.frag) glDeleteShader(handle.frag);
    handle = ShaderProgramHandle{};
}

void oglUseShaderProgram(ShaderProgramHandle& handle) {
    glUseProgram(handle.program);
}

int oglShaderGetUniformLocation(ShaderProgramHandle& handle, const std::string& name) {
    return glGetUniformLocation(handle.program, name.c_str());
}

void oglSetUniformBool(int location, bool value) {
    glUniform1i(location, value ? 1 : 0);
}

void oglSetUniformInt(int location, int value) {
    glUniform1i(location, value);
}

void oglSetUniformFloat(int location, float value) {
    glUniform1f(location, value);
}

void oglSetUniformVec2(int location, vec2 value) {
    glUniform2f(location, value.x, value.y);
}

void oglSetUniformVec3(int location, vec3 value) {
    glUniform3f(location, value.x, value.y, value.z);
}

void oglSetUniformVec4(int location, vec4 value) {
    glUniform4f(location, value.x, value.y, value.z, value.w);
}

void oglSetUniformMat4(int location, mat4 value) {
    glUniformMatrix4fv(location, 1, GL_TRUE, &value.a.a[0][0]);
}

}
