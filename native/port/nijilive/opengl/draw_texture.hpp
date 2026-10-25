#pragma once
#include "shader.hpp"
#include <core/render/common.hpp>
#include <core/math/camera.hpp>
namespace nijiexpose::ogl {
using nicxlive::core::Texture;
using nicxlive::core::Rect;
using nicxlive::core::nodes::Part;
using nicxlive::core::math::Camera;
void oglDrawTextureAtPart(const Texture* texture, const std::shared_ptr<Part>& part);
void oglDrawTextureAtPosition(const Texture* texture, vec2 position, float opacity = 1,
    vec3 color = {1,1,1}, vec3 screenColor = {0,0,0});
void oglDrawTextureAtRect(const Texture* texture, Rect area, Rect uvs = {0,0,1,1}, float opacity = 1,
    vec3 color = {1,1,1}, vec3 screenColor = {0,0,0}, Shader shader = {}, Camera* camera = nullptr);
}
