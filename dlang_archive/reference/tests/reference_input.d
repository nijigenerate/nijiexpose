import nijiui.input;
import bindbc.imgui;
import std.stdio;

void main() {
    igCreateContext(null);
    auto io = igGetIO();
    io.IniFilename = null;
    io.DisplaySize = ImVec2(1280, 720);
    io.DeltaTime = 1.0f / 60;
    char* pixels;
    int width, height;
    ImFontAtlas_GetTexDataAsRGBA32(io.Fonts, &pixels, &width, &height);
    foreach (frame; 0..8) {
        ImGuiIO_AddMousePosEvent(io, 100 + frame*8, 150 + frame*3);
        ImGuiIO_AddMouseButtonEvent(io, 0, frame >= 1 && frame <= 3);
        ImGuiIO_AddMouseButtonEvent(io, 1, frame == 4 || frame == 5);
        ImGuiIO_AddMouseButtonEvent(io, 2, frame == 6);
        igNewFrame();
        igGetCurrentContext().HoveredWindow = frame % 2 == 1 ? cast(ImGuiWindow*)1 : null;
        inInputUpdate();
        foreach (button; [MouseButton.Left, MouseButton.Middle, MouseButton.Right]) {
            writef("%d%d%d%d%d%d ", inInputMouseDown(button), inInputWasMouseDown(button),
                inInputMouseDownBegannijiui(button), inInputMouseClicked(button),
                inInputMouseDoubleClicked(button), inInputMouseDragging(button));
            auto drag = inInputMouseDragDelta(button);
            writef("%.0f,%.0f ", drag.x, drag.y);
        }
        auto position = inInputMousePosition();
        writefln("%.0f,%.0f %.0f", position.x, position.y, inInputMouseScrollDelta());
        igGetCurrentContext().HoveredWindow = null;
        igEndFrame();
    }
    igDestroyContext(null);
}
