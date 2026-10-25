#pragma once
#include "shader.hpp"
#include <vector>
namespace nijiexpose::ogl {
void oglInitDebugRenderer();
void oglSetDebugPointSize(float size);
void oglSetDebugLineWidth(float size);
void oglUploadDebugBuffer(const std::vector<vec3>& points, std::span<const uint16_t> indices);
void oglSetDebugExternalBuffer(GLuint vertexBuffer, GLuint indexBuffer, int count);
void oglDrawDebugPoints(vec4 color, mat4 mvp);
void oglDrawDebugLines(vec4 color, mat4 mvp);
}
