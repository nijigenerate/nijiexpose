// nijiui/input.d: preserve source indexing and update order (including Middle/Right).
#include "nijiui/input.hpp"
#include <imgui_internal.h>
namespace nijiui {
namespace {
thread_local bool wasMouseDown[3]{};
thread_local bool isMouseDown[3]{};
thread_local bool downBeganInWindow[3]{};
}
void inInputUpdate() {
    wasMouseDown[0] = isMouseDown[0];
    wasMouseDown[1] = isMouseDown[1];
    wasMouseDown[2] = isMouseDown[2];
    isMouseDown[0] = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    isMouseDown[1] = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    isMouseDown[2] = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    if (!wasMouseDown[0] && isMouseDown[0] && inInputIsnijiui()) downBeganInWindow[0] = true;
    if (!wasMouseDown[1] && isMouseDown[1] && inInputIsnijiui()) downBeganInWindow[1] = true;
    if (!wasMouseDown[2] && isMouseDown[2] && inInputIsnijiui()) downBeganInWindow[2] = true;
    if (!isMouseDown[0]) downBeganInWindow[0] = false;
    if (!isMouseDown[1]) downBeganInWindow[1] = false;
    if (!isMouseDown[2]) downBeganInWindow[2] = false;
}
bool inInputMouseDownBegannijiui(MouseButton button) { return isMouseDown[int(button)] && downBeganInWindow[int(button)]; }
bool inInputIsnijiui() { return ImGui::GetCurrentContext()->HoveredWindow != nullptr; }
bool inInputMouseDown(MouseButton button) { return isMouseDown[int(button)]; }
bool inInputWasMouseDown(MouseButton button) { return wasMouseDown[int(button)]; }
bool inInputMouseClicked(MouseButton button, bool repeat) { return ImGui::IsMouseClicked(int(button), repeat); }
bool inInputMouseDoubleClicked(MouseButton button) { return ImGui::IsMouseDoubleClicked(int(button)); }
bool inInputMouseDragging(MouseButton button) { return ImGui::IsMouseDragging(int(button)); }
vec2 inInputMouseDragDelta(MouseButton button, float lockThreshold) {
    const auto value = ImGui::GetMouseDragDelta(int(button), lockThreshold);
    return {value.x, value.y};
}
vec2 inInputMousePosition() { const auto value = ImGui::GetMousePos(); return {value.x, value.y}; }
float inInputMouseScrollDelta() { return ImGui::GetIO().MouseWheel; }
}
