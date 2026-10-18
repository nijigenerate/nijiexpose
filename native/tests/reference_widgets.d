import nijiui.widgets.input;
import nijiui.widgets.label;
import nijiui.widgets.dummy;
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
    bool toggle;
    string buffer = "";
    ImVec2 switchPosition = ImVec2(0, 0);
    foreach (frame; 0..7) {
        ImGuiIO_AddMousePosEvent(io, switchPosition.x, switchPosition.y);
        ImGuiIO_AddMouseButtonEvent(io, 0, frame == 1);
        if (frame == 5) ImGuiIO_AddInputCharactersUTF8(io, "abc");
        igNewFrame();
        igSetNextWindowPos(ImVec2(20, 20), ImGuiCond.Always, ImVec2(0, 0));
        igSetNextWindowSize(ImVec2(600, 600), ImGuiCond.Always);
        igBegin("widgets parity", null, ImGuiWindowFlags.NoSavedSettings);
        uiImLabel("plain text");
        uiImLabelColored("colored", vec4(0.2, 0.4, 0.6, 1));
        uiImLabelDisabled("disabled");
        uiImLabelWrapped("wrapped text", 200);
        igGetCursorScreenPos(&switchPosition);
        switchPosition.x += 10;
        switchPosition.y += 10;
        auto changed = uiImCheckbox("toggle###switch", toggle);
        uiImButton("button", vec2(80, 30));
        float cmin = -0.5f, cmax = 0.5f;
        uiImPushTextWrapPos(300);
        uiImPopTextWrapPos();
        uiImRange(cmin, cmax, -1, 1);
        if (frame == 3) igSetKeyboardFocusHere();
        auto edited = uiImInputText("edit", "label", 180, buffer);
        igEnd();
        igRender();
        auto data = igGetDrawData();
        ulong hash = 14695981039346656037UL;
        foreach (i; 0..data.CmdListsCount) {
            auto list = data.CmdLists[i];
            hash = hashBytes(hash, cast(const(ubyte)*)list.VtxBuffer.Data, list.VtxBuffer.Size * ImDrawVert.sizeof);
            hash = hashBytes(hash, cast(const(ubyte)*)list.IdxBuffer.Data, list.IdxBuffer.Size * ImDrawIdx.sizeof);
        }
        writef("%d %d %d %d %016x ", toggle, changed, edited, data.TotalVtxCount, hash);
        foreach (ubyte c; cast(immutable(ubyte)[])buffer) writef("%02x", c);
        writeln();
    }
    igDestroyContext(null);
}
