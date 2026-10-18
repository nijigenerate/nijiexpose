// Port of nijiui/widgets/image.d.
#include "image.hpp"
namespace nijiui {
void uiImImage(uint32_t id,vec2 size,ImageRect cut,vec4 tint,vec4 border) {
#if IMGUI_VERSION_NUM >= 19200
    ImGui::PushStyleColor(ImGuiCol_Border,{border.x,border.y,border.z,border.w});
    ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize,border.w>0 ? 1.f : 0.f);
    ImGui::ImageWithBg(static_cast<ImTextureID>(id),{size.x,size.y},{cut.x,cut.y},{cut.x+cut.width,cut.y+cut.height},{0,0,0,0},{tint.x,tint.y,tint.z,tint.w});
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
#else
    ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(id)),{size.x,size.y},{cut.x,cut.y},{cut.x+cut.width,cut.y+cut.height},{tint.x,tint.y,tint.z,tint.w},{border.x,border.y,border.z,border.w});
#endif
}
}
