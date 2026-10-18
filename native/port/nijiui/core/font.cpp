// Port of nijiui/core/font.d, using ImGui's current font-loader API.
#include "font.hpp"
#include "settings.hpp"
#include <imgui.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <span>
#include <stdexcept>
namespace nijiui {
namespace {
thread_local ImFontAtlas* atlas;
std::span<std::byte> fontResource(int id) {
    auto module=GetModuleHandleW(nullptr);
    auto resource=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));
    if(!resource) throw std::runtime_error("Missing embedded nijiui font resource");
    auto data=LoadResource(module,resource);
    if(!data) throw std::runtime_error("Cannot load embedded nijiui font resource");
    return {static_cast<std::byte*>(LockResource(data)),SizeofResource(module,resource)};
}
void inAddFontData(const char* name,std::span<std::byte> data,float size,const ImWchar* ranges,ImVec2 offset) {
    ImFontConfig cfg;
#if IMGUI_VERSION_NUM >= 19200
    cfg.FontLoaderFlags=1<<9;
#else
    cfg.FontBuilderFlags=1<<9;
#endif
    cfg.FontDataOwnedByAtlas=false;
    cfg.MergeMode=!atlas->Fonts.empty();
    cfg.GlyphOffset=offset;
    cfg.OversampleH=3;
    cfg.OversampleV=2;
    std::memset(cfg.Name,0,sizeof(cfg.Name));
    std::memcpy(cfg.Name,name,std::strlen(name));
    atlas->AddFontFromMemoryTTF(data.data(),static_cast<int>(data.size()),size,&cfg,ranges);
}
void fontScale(float scale) {
#if IMGUI_VERSION_NUM >= 19200
    ImGui::GetStyle().FontScaleMain=scale;
#else
    ImGui::GetIO().FontGlobalScale=scale;
#endif
}
}
void inInitFonts() {
    atlas=ImGui::GetIO().Fonts;
    static constexpr ImWchar notoRanges[]{0x20,0xFF,0x2000,0x206F,0x3000,0x30FF,0x31F0,0x31FF,0xFF00,0xFFEF,0xFFFD,0xFFFD,0x4e00,0x9FAF,0};
    // New ImGui requires a persistent, terminated range array.
    static constexpr ImWchar iconRanges[]{0xE000,0xF23B,0};
    inAddFontData("APP",fontResource(4101),26,notoRanges,{0,-6});
    inAddFontData("Icons",fontResource(4102),32,iconRanges,{0,-2});
#if IMGUI_VERSION_NUM < 19200
    atlas->Build();
#endif
    // ImGui >=1.92 uploads the dynamic atlas through the official renderer backend.
    inSetUIScale(inGetUIScale());
}
void inSetUIScale(float scale) { inSettingsSet("UIScale",scale); fontScale(inGetUIScaleFont()); }
float inGetUIScaleFont() { return inGetUIScale()/2; }
float inGetUIScale() { return inSettingsGet<float>("UIScale",1.0f); }
std::string inGetUIScaleText() { return std::to_string(static_cast<int>(inGetUIScale()*100))+"%"; }
void inFontsBeginLarge() { fontScale(inGetUIScaleFont()*2); }
void inFontsEndLarge() { fontScale(inGetUIScaleFont()); }
}
