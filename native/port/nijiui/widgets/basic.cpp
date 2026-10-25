// Direct translations of nijiui/widgets/{dummy,label,menu,popup,package,progress}.d.
// Copyright (c) 2022 Inochi2D Project, 2024 nijigenerate Project. BSD-2-Clause.
#include "nijiui/widgets/basic.hpp"
#include <imgui_internal.h>
#include <cmath>
namespace nijiui {
void uiImDummy(vec2 size) {
    vec2 avail = uiImAvailableSpace();
    if (size.x <= 0) size.x = avail.x - std::abs(size.x);
    if (size.y <= 0) size.y = avail.y - std::abs(size.y);
    ImGui::Dummy(ImVec2(size.x, size.y));
}
void uiImSpacer(vec2 size) { ImGui::SameLine(0, 0); uiImDummy(size); }
void uiImSameLine(float startOffset, float spacing) { ImGui::SameLine(startOffset, spacing); }
vec2 uiImAvailableSpace() { const auto avail = ImGui::GetContentRegionAvail(); return {avail.x, avail.y}; }
vec2 uiImMeasureString(std::string_view text) {
    const auto size = ImGui::CalcTextSize(text.data(), text.data() + text.size());
    return {size.x, size.y};
}
void uiImLabel(std::string_view text) { ImGui::TextEx(text.data(), text.data() + text.size()); }
void uiImLabelColored(std::string_view text, vec4 color) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.x, color.y, color.z, color.w));
    ImGui::TextEx(text.data(), text.data() + text.size());
    ImGui::PopStyleColor();
}
void uiImLabelDisabled(std::string_view text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
    ImGui::PopStyleColor();
}
void uiImLabelWrapped(std::string_view text, float wrapPosition) {
    uiImPushTextWrapPos(wrapPosition);
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
    uiImPopTextWrapPos();
}
void uiImPushTextWrapPos(float wrapPosition) { ImGui::PushTextWrapPos(wrapPosition); }
void uiImPopTextWrapPos() { ImGui::PopTextWrapPos(); }
void uiImTooltip(std::string_view tip) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        uiImPushTextWrapPos(ImGui::GetFontSize() * 35);
        ImGui::TextUnformatted(tip.data(), tip.data() + tip.size());
        uiImPopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::PopStyleVar();
}
bool uiImBeginMainMenuBar() { return ImGui::BeginMainMenuBar(); }
void uiImEndMainMenuBar() { ImGui::EndMainMenuBar(); }
bool uiImBeginMenuBar() { return ImGui::BeginMenuBar(); }
void uiImEndMenuBar() { ImGui::EndMenuBar(); }
bool uiImBeginMenu(const char* label, bool enabled) { return ImGui::BeginMenu(label, enabled); }
void uiImEndMenu() { ImGui::EndMenu(); }
bool uiImMenuItem(const char* label, const char* shortcut, bool selected, bool enabled) { return ImGui::MenuItem(label, shortcut, selected, enabled); }
bool uiImBeginPopup(std::string_view id) {
    return ImGui::BeginPopupEx(ImGui::GetID(id.data(), id.data() + id.size()),
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
}
void uiImEndPopup() { ImGui::EndPopup(); }
void uiImOpenPopup(std::string_view id) { ImGui::OpenPopup(ImGui::GetID(id.data(), id.data() + id.size())); }
void uiImRightClickPopup(std::string_view id) { if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) uiImOpenPopup(id); }
void uiImPush(int id) { ImGui::PushID(id); }
void uiImPush(std::string_view id) { ImGui::PushID(id.data(), id.data() + id.size()); }
void uiImPop() { ImGui::PopID(); }
void uiImIndent(float indentW) { ImGui::Indent(indentW); }
void uiImUnindent() { ImGui::Unindent(); }
void uiImSeperator() {
    ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::Separator();
    ImGui::PopStyleColor();
}
void uiImNewLine() { ImGui::NewLine(); }
bool uiImHeader(const char* label, bool defaultOpen) { return ImGui::CollapsingHeader(label, defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None); }
bool uiImBeginChild(std::string_view id, vec2 size, bool border) {
    return ImGui::BeginChild(ImGui::GetID(id.data(), id.data() + id.size()), ImVec2(size.x, size.y), border);
}
void uiImEndChild() { ImGui::EndChild(); }
void uiImProgress(float progress, vec2 size, const char* text) { ImGui::ProgressBar(progress, ImVec2(size.x, size.y), text); }
}
