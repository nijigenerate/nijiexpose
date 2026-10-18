#pragma once
#include "resources.hpp"
#include <memory>
namespace nijiexpose::ogl {
struct ShaderAsset { std::string vertex, fragment; };
class Shader {
    struct Program {
        ShaderProgramHandle handle;
        ~Program() { oglDestroyShaderProgram(handle); }
    };
    std::shared_ptr<Program> program;
public:
    Shader() = default;
    explicit Shader(const ShaderAsset& source) : Shader(source.vertex, source.fragment) {}
    Shader(const std::string& vertex, const std::string& fragment) : program(std::make_shared<Program>()) {
        oglCreateShaderProgram(program->handle, vertex, fragment);
    }
    explicit operator bool() const { return static_cast<bool>(program); }
    void use() { if (program) oglUseShaderProgram(program->handle); }
    int getUniformLocation(const std::string& name) { return program ? oglShaderGetUniformLocation(program->handle, name) : -1; }
    void setUniform(int location, bool value) { if (program) oglSetUniformBool(location, value); }
    void setUniform(int location, int value) { if (program) oglSetUniformInt(location, value); }
    void setUniform(int location, float value) { if (program) oglSetUniformFloat(location, value); }
    void setUniform(int location, vec2 value) { if (program) oglSetUniformVec2(location, value); }
    void setUniform(int location, vec3 value) { if (program) oglSetUniformVec3(location, value); }
    void setUniform(int location, vec4 value) { if (program) oglSetUniformVec4(location, value); }
    void setUniform(int location, mat4 value) { if (program) oglSetUniformMat4(location, value); }
};
}
