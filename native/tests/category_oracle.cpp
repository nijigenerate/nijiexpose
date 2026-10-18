#include "nijiui/widgets/category.hpp"
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
    io.DisplaySize = ImVec2(1280,720);
    io.DeltaTime = 1.0f/60;
#ifdef NIJIEXPOSE_MODERN_IMGUI_TEST
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
#else
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
#endif
    ImVec2 target(0,0);
    for (int frame = 0; frame < 8; ++frame) {
        io.AddMousePosEvent(target.x, target.y);
        io.AddMouseButtonEvent(0, frame == 2);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(20,20), ImGuiCond_Always, ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2(600,600), ImGuiCond_Always);
        ImGui::Begin("category parity", nullptr, ImGuiWindowFlags_NoSavedSettings);
        target = ImGui::GetCursorScreenPos(); target.x += 12; target.y += 8;
        auto* window = ImGui::GetCurrentWindow();
        const auto work = window->WorkRect.Min.x, content = window->ContentRegionRect.Min.x;
        const auto a = uiImBeginCategory("normal", ImVec4(0.2f,0.4f,0.6f,0.35f));
        if (a) uiImLabel("content");
        uiImEndCategory();
        const auto b = uiImBeginCategory("fixed", ImVec4(0.9f,0.9f,0.9f,0.84f), UiImCategoryFlags::NoCollapse);
        if (b) uiImDummy({20,30});
        uiImEndCategory();
        const auto c = uiImBeginCategory("closed", ImVec4(0.05f,0.1f,0.14f,1.2f), UiImCategoryFlags::DefaultClosed);
        if (c) uiImLabel("closed content");
        uiImEndCategory();
        if (window->WorkRect.Min.x != work || window->ContentRegionRect.Min.x != content) return 1;
#ifdef NIJIEXPOSE_MODERN_IMGUI_TEST
        if (!b || c || (frame >= 4 && a)) return 2;
#endif
        ImGui::End();
        ImGui::Render();
        const auto* data = ImGui::GetDrawData();
        std::uint64_t hash = 14695981039346656037ULL;
#ifdef NIJIEXPOSE_MODERN_IMGUI_TEST
        const int count = data->CmdLists.Size;
#else
        const int count = data->CmdListsCount;
#endif
        for (int i = 0; i < count; ++i) {
            const auto* list = data->CmdLists[i];
            hash = hashBytes(hash, reinterpret_cast<const unsigned char*>(list->VtxBuffer.Data), list->VtxBuffer.Size * sizeof(ImDrawVert));
            hash = hashBytes(hash, reinterpret_cast<const unsigned char*>(list->IdxBuffer.Data), list->IdxBuffer.Size * sizeof(ImDrawIdx));
        }
        std::printf("%d%d%d %d %016llx\n", a,b,c,data->TotalVtxCount, static_cast<unsigned long long>(hash));
    }
#ifdef NIJIEXPOSE_MODERN_IMGUI_TEST
    for (auto* texture : ImGui::GetPlatformIO().Textures) texture->SetStatus(ImTextureStatus_Destroyed);
#endif
    ImGui::DestroyContext();
}
