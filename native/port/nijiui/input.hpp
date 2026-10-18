#pragma once
#include <imgui.h>
#include <math/types.hpp>
namespace nijiui {
using vec2 = nicxlive::core::math::Vec2;
using vec3 = nicxlive::core::math::Vec3;
using vec4 = nicxlive::core::math::Vec4;
enum class MouseButton { Left = ImGuiMouseButton_Left, Middle = ImGuiMouseButton_Middle, Right = ImGuiMouseButton_Right };
void inInputUpdate();
bool inInputMouseDownBegannijiui(MouseButton button);
bool inInputIsnijiui();
bool inInputMouseDown(MouseButton button);
bool inInputWasMouseDown(MouseButton button);
bool inInputMouseClicked(MouseButton button, bool repeat = false);
bool inInputMouseDoubleClicked(MouseButton button);
bool inInputMouseDragging(MouseButton button);
vec2 inInputMouseDragDelta(MouseButton button, float lockThreshold = -1.0f);
vec2 inInputMousePosition();
float inInputMouseScrollDelta();
}
