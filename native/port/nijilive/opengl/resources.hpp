#pragma once
#include "api.hpp"
#include <core/math/mat4.hpp>
#include <core/nodes/common.hpp>
#include <span>
#include <string>
namespace nijiexpose::ogl {
using GLId = GLuint;
using nicxlive::core::math::Mat4;
using mat4 = Mat4;
using vec2 = nicxlive::core::math::Vec2;
using vec3 = nicxlive::core::math::Vec3;
using vec4 = nicxlive::core::math::Vec4;
using BlendMode = nicxlive::core::nodes::BlendMode;
enum class Filtering { Linear, Nearest };
// nicxlive's public Wrapping currently omits Mirror; retain the D renderer mode.
enum class Wrapping { Clamp, Repeat, Mirror };
struct ShaderProgramHandle { GLuint program{}, vert{}, frag{}; };
void oglCreateTextureHandle(GLId& id);
void oglDeleteTextureHandle(GLId& id);
void oglBindTextureHandle(GLId id, unsigned unit);
void oglUploadTextureData(GLId id, int width, int height, int inChannels, int outChannels, bool stencil, std::span<const uint8_t> data);
void oglUpdateTextureRegion(GLId id, int x, int y, int width, int height, int channels, std::span<const uint8_t> data);
void oglGenerateTextureMipmap(GLId id);
void oglApplyTextureFiltering(GLId id, Filtering filtering, bool useMipmaps);
void oglApplyTextureWrapping(GLId id, Wrapping wrapping);
void oglApplyTextureAnisotropy(GLId id, float value);
float oglMaxTextureAnisotropy();
void oglReadTextureData(GLId id, int channels, bool stencil, std::span<uint8_t> buffer);
void oglCreateShaderProgram(ShaderProgramHandle& handle, const std::string& vertex, const std::string& fragment);
void oglDestroyShaderProgram(ShaderProgramHandle& handle);
void oglUseShaderProgram(ShaderProgramHandle& handle);
int oglShaderGetUniformLocation(ShaderProgramHandle& handle, const std::string& name);
void oglSetUniformBool(int location, bool value);
void oglSetUniformInt(int location, int value);
void oglSetUniformFloat(int location, float value);
void oglSetUniformVec2(int location, vec2 value);
void oglSetUniformVec3(int location, vec3 value);
void oglSetUniformVec4(int location, vec4 value);
void oglSetUniformMat4(int location, mat4 value);
void oglSetAdvancedBlendCoherent(bool enable);
void oglSetLegacyBlendMode(BlendMode blendingMode);
void oglSetAdvancedBlendEquation(BlendMode blendingMode);
}
