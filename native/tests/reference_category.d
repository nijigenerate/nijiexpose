import reference_category_impl;
import nijiui.widgets.dummy;
import nijiui.widgets.label;
import bindbc.imgui;
import std.stdio;
import inmath;
ulong hashBytes(ulong hash, const(ubyte)* bytes, size_t size) {
    foreach (i; 0..size) { hash ^= bytes[i]; hash *= 1099511628211UL; }
    return hash;
}
void main() {
    igCreateContext(null);
    auto io = igGetIO();
    io.IniFilename = null;
    io.DisplaySize = ImVec2(1280, 720);
    io.DeltaTime = 1.0f/60;
    char* pixels;
    int width, height;
    ImFontAtlas_GetTexDataAsRGBA32(io.Fonts, &pixels, &width, &height);
    ImVec2 target = ImVec2(0,0);
    foreach (frame; 0..8) {
        ImGuiIO_AddMousePosEvent(io, target.x, target.y);
        ImGuiIO_AddMouseButtonEvent(io, 0, frame == 2);
        igNewFrame();
        igSetNextWindowPos(ImVec2(20, 20), ImGuiCond.Always, ImVec2(0,0));
        igSetNextWindowSize(ImVec2(600, 600), ImGuiCond.Always);
        igBegin("category parity", null, ImGuiWindowFlags.NoSavedSettings);
        igGetCursorScreenPos(&target); target.x += 12; target.y += 8;
        auto window = igGetCurrentWindow();
        const work = window.WorkRect.Min.x, content = window.ContentRegionRect.Min.x;
        const a = uiImBeginCategory("normal", ImVec4(0.2,0.4,0.6,0.35));
        if (a) uiImLabel("content");
        uiImEndCategory();
        const b = uiImBeginCategory("fixed", ImVec4(0.9,0.9,0.9,0.84), UiImCategoryFlags.NoCollapse);
        if (b) uiImDummy(vec2(20, 30));
        uiImEndCategory();
        const c = uiImBeginCategory("closed", ImVec4(0.05,0.1,0.14,1.2), UiImCategoryFlags.DefaultClosed);
        if (c) uiImLabel("closed content");
        uiImEndCategory();
        assert(window.WorkRect.Min.x == work && window.ContentRegionRect.Min.x == content);
        igEnd();
        igRender();
        auto data = igGetDrawData();
        ulong hash = 14695981039346656037UL;
        foreach (i; 0..data.CmdListsCount) {
            auto list = data.CmdLists[i];
            hash = hashBytes(hash, cast(const(ubyte)*)list.VtxBuffer.Data, list.VtxBuffer.Size * ImDrawVert.sizeof);
            hash = hashBytes(hash, cast(const(ubyte)*)list.IdxBuffer.Data, list.IdxBuffer.Size * ImDrawIdx.sizeof);
        }
        writefln("%d%d%d %d %016x", a, b, c, data.TotalVtxCount, hash);
    }
    igDestroyContext(null);
}
