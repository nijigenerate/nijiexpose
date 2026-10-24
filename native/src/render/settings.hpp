#pragma once
#include <core/math/mat4.hpp>
namespace nijiexpose::render {
// Scene setting shared by the application and the selected renderer.
inline thread_local nicxlive::core::math::Vec3 inSceneAmbientLight{1, 1, 1};
} // namespace nijiexpose::render
