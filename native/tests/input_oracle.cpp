#include "nijiui/input.hpp"
#include <imgui_internal.h>
#include <cstdio>
#include <initializer_list>
int main() {
    using namespace nijiui;
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1280, 720);
    io.DeltaTime = 1.0f / 60;
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    for (int frame = 0; frame < 8; ++frame) {
        io.AddMousePosEvent(float(100 + frame*8), float(150 + frame*3));
        io.AddMouseButtonEvent(0, frame >= 1 && frame <= 3);
        io.AddMouseButtonEvent(1, frame == 4 || frame == 5);
        io.AddMouseButtonEvent(2, frame == 6);
        ImGui::NewFrame();
        ImGui::GetCurrentContext()->HoveredWindow = frame % 2 == 1 ? reinterpret_cast<ImGuiWindow*>(1) : nullptr;
        inInputUpdate();
        for (const auto button : {MouseButton::Left, MouseButton::Middle, MouseButton::Right}) {
            std::printf("%d%d%d%d%d%d ", inInputMouseDown(button), inInputWasMouseDown(button),
                inInputMouseDownBegannijiui(button), inInputMouseClicked(button),
                inInputMouseDoubleClicked(button), inInputMouseDragging(button));
            const auto drag = inInputMouseDragDelta(button);
            std::printf("%.0f,%.0f ", drag.x, drag.y);
        }
        const auto position = inInputMousePosition();
        std::printf("%.0f,%.0f %.0f\n", position.x, position.y, inInputMouseScrollDelta());
        ImGui::GetCurrentContext()->HoveredWindow = nullptr;
        ImGui::EndFrame();
    }
    ImGui::DestroyContext();
}
