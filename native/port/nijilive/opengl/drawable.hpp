#pragma once
#include "api.hpp"
#include <core/math/veca.hpp>
#include <span>
namespace nijiexpose::ogl {
using nicxlive::core::math::Vec2Array;
void oglInitDrawableBackend();
void oglBindDrawableVao();
void oglCreateDrawableBuffers(GLuint& ibo);
void oglUploadDrawableIndices(GLuint ibo, std::span<const uint16_t> indices);
void oglUploadSharedVertexBuffer(const Vec2Array& vertices);
void oglUploadSharedUvBuffer(const Vec2Array& uvs);
void oglUploadSharedDeformBuffer(const Vec2Array& deformation);
GLuint oglGetSharedVertexBuffer();
GLuint oglGetSharedUvBuffer();
GLuint oglGetSharedDeformBuffer();
void oglDrawDrawableElements(GLuint ibo, size_t indexCount);
}
