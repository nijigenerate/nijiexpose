#include "nijiui/widgets/input.hpp"
#include "windows/utils.hpp"
#include <imgui_internal.h>
#include <cstdint>
#include <cstdio>

std::uint64_t hashBytes(std::uint64_t hash, const unsigned char* bytes, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) { hash ^= bytes[i]; hash *= 1099511628211ULL; }
    return hash;
}
int main() {
    using namespace nijiui;
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1280, 720);
    io.DeltaTime = 1.0f/60;
#ifdef NIJIEXPOSE_MODERN_IMGUI_TEST
    if (std::string_view(ImGui::GetVersion()) != "1.93.0") return 1;
    nijiexpose::neSetStyle();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
#else
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
#endif
    bool toggle = false;
    std::string buffer = "";
    ImVec2 switchPosition(0, 0);
    for (int frame = 0; frame < 7; ++frame) {
        io.AddMousePosEvent(switchPosition.x, switchPosition.y);
        io.AddMouseButtonEvent(0, frame == 1);
        if (frame == 5) io.AddInputCharactersUTF8("abc");
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always, ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_Always);
        ImGui::Begin("widgets parity", nullptr, ImGuiWindowFlags_NoSavedSettings);
        uiImLabel("plain text");
        uiImLabelColored("colored", {0.2f, 0.4f, 0.6f, 1});
        uiImLabelDisabled("disabled");
        uiImLabelWrapped("wrapped text", 200);
        switchPosition = ImGui::GetCursorScreenPos();
        switchPosition.x += 10;
        switchPosition.y += 10;
        const auto changed = uiImCheckbox("toggle###switch", toggle);
        uiImButton("button", {80, 30});
        float cmin = -0.5f, cmax = 0.5f;
        uiImPushTextWrapPos(300);
        uiImPopTextWrapPos();
        uiImRange(cmin, cmax, -1, 1);
        if (frame == 3) ImGui::SetKeyboardFocusHere();
        const auto edited = uiImInputText("edit", "label", 180, buffer);
        ImGui::End();
        ImGui::Render();
        const auto* data = ImGui::GetDrawData();
#ifdef NIJIEXPOSE_MODERN_IMGUI_TEST
        if (data->TotalVtxCount <= 0 || toggle != (frame >= 2) ||
            changed != (frame == 2) || edited != (frame == 5)) return 2;
        if (frame >= 5 && std::string_view(buffer.c_str()) != "abc") return 3;
#endif
        std::uint64_t hash = 14695981039346656037ULL;
#ifdef NIJIEXPOSE_MODERN_IMGUI_TEST
        const int drawListCount = data->CmdLists.Size;
#else
        const int drawListCount = data->CmdListsCount;
#endif
        for (int i = 0; i < drawListCount; ++i) {
            const auto* list = data->CmdLists[i];
            hash = hashBytes(hash, reinterpret_cast<const unsigned char*>(list->VtxBuffer.Data), list->VtxBuffer.Size * sizeof(ImDrawVert));
            hash = hashBytes(hash, reinterpret_cast<const unsigned char*>(list->IdxBuffer.Data), list->IdxBuffer.Size * sizeof(ImDrawIdx));
        }
        std::printf("%d %d %d %d %016llx ", toggle, changed, edited, data->TotalVtxCount, static_cast<unsigned long long>(hash));
        for (const unsigned char value : buffer) std::printf("%02x", value);
        std::puts("");
    }
#ifdef NIJIEXPOSE_MODERN_IMGUI_TEST
    // A headless test has no GPU resources to release.
    for (auto* texture : ImGui::GetPlatformIO().Textures)
        texture->SetStatus(ImTextureStatus_Destroyed);
#endif
    ImGui::DestroyContext();
}
