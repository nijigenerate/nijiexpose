// nijiui/widgets/input.d, translated in source order (color storage is colors.cpp).
// Copyright (c) 2022 Inochi2D Project, 2024 nijigenerate Project. BSD-2-Clause.
#include "nijiui/widgets/input.hpp"
#include "dcompat/scope.hpp"
#include "dcompat/string.hpp"
#include <imgui_internal.h>
#include <SDL3/SDL.h>
namespace nijiui {
namespace { struct TextCallbackUserData { std::string* str; }; }

bool uiImInputText(std::string_view id, std::string& buffer, ImGuiInputTextFlags flags) {
    return uiImInputText(id, uiImAvailableSpace().x, buffer, flags);
}
bool uiImInputText(std::string_view wId, float width, std::string& buffer, ImGuiInputTextFlags flags) {
    if (buffer.empty() || buffer[buffer.size() - 1] != '\0') buffer += '\0';
    ImGui::PushID(int(ImGui::GetID(wId.data(), wId.data() + wId.size())));
    nijiexpose::dcompat::ScopeExit popID([] { ImGui::PopID(); });
    TextCallbackUserData cb;
    cb.str = &buffer;
    ImGui::PushItemWidth(width);
    nijiexpose::dcompat::ScopeExit popWidth([] { ImGui::PopItemWidth(); });
    if (ImGui::InputText("###INPUT", buffer.data(), buffer.size() + 1,
        flags | ImGuiInputTextFlags_CallbackResize, [](ImGuiInputTextCallbackData* data) {
            auto* udata = static_cast<TextCallbackUserData*>(data->UserData);
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                if (data->BufTextLen < 0) data->BufTextLen = 0;
                udata->str->resize(data->BufTextLen + 1);
                data->Buf = udata->str->data();
                data->Buf[data->BufTextLen] = '\0';
                udata->str->resize(udata->str->size() - 1);
            }
            return 0;
        }, &cb)) {
        buffer = std::string(nijiexpose::dcompat::fromStringz(buffer));
        return true;
    }
    const auto min = ImGui::GetItemRectMin();
    const auto max = ImGui::GetItemRectMax();
    const SDL_Rect rect{int(min.x) + 32, int(min.y), int(max.x), 32};
    // SDL3 replacement for the original SDL_SetTextInputRect global-focus API.
    SDL_SetTextInputArea(SDL_GetKeyboardFocus(), &rect, 0);
    return false;
}
bool uiImInputText(std::string_view id, std::string_view label, std::string& buffer, ImGuiInputTextFlags flags) {
    return uiImInputText(id, label, uiImAvailableSpace().x, buffer, flags);
}
bool uiImInputText(std::string_view wId, std::string_view label, float width, std::string& buffer, ImGuiInputTextFlags flags) {
    if (buffer.empty() || buffer[buffer.size() - 1] != '\0') buffer += '\0';
    ImGui::PushID(int(ImGui::GetID(wId.data(), wId.data() + wId.size())));
    nijiexpose::dcompat::ScopeExit popID([] { ImGui::PopID(); });
    TextCallbackUserData cb;
    cb.str = &buffer;
    ImGui::PushItemWidth(width);
    nijiexpose::dcompat::ScopeExit popWidth([] { ImGui::PopItemWidth(); });
    nijiexpose::dcompat::ScopeSuccess renderLabel([&] {
        ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x);
        ImGui::TextEx(label.data(), label.data() + label.size());
    });
    if (ImGui::InputText("###INPUT", buffer.data(), buffer.size() + 1,
        flags | ImGuiInputTextFlags_CallbackResize, [](ImGuiInputTextCallbackData* data) {
            auto* udata = static_cast<TextCallbackUserData*>(data->UserData);
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                if (data->BufTextLen < 0) data->BufTextLen = 0;
                udata->str->resize(data->BufTextLen + 1);
                data->Buf = udata->str->data();
                data->Buf[data->BufTextLen] = '\0';
                udata->str->resize(udata->str->size() - 1);
            }
            return 0;
        }, &cb)) {
        buffer = std::string(nijiexpose::dcompat::fromStringz(buffer));
        return true;
    }
    const auto min = ImGui::GetItemRectMin();
    const auto max = ImGui::GetItemRectMax();
    const SDL_Rect rect{int(min.x) + 32, int(min.y), int(max.x), 32};
    SDL_SetTextInputArea(SDL_GetKeyboardFocus(), &rect, 0);
    return false;
}
bool uiImCheckbox(const char* text, bool& val) {
    const float height = 22.0f, width = 38.0f, knobSize = 18.0f;
    const float knobPadding = (height - knobSize) * 0.5f;
    const std::string_view label = text ? text : "";
    const auto idIndex = label.find("###");
    const auto visibleLabel = idIndex != std::string_view::npos ? label.substr(0, idIndex) : label;
    ImGui::PushID(text);
    nijiexpose::dcompat::ScopeExit popID([] { ImGui::PopID(); });
    const auto startPos = ImGui::GetCursorScreenPos();
    const bool changed = ImGui::InvisibleButton("##switch", ImVec2(width, height));
    const bool hovered = ImGui::IsItemHovered();
    if (changed) val = !val;
    auto* drawList = ImGui::GetWindowDrawList();
    auto trackColor = val ? ImVec4(0.70f, 0.25f, 0.00f, 1.00f) : ImVec4(0.78f, 0.81f, 0.86f, 1.00f);
    if (hovered && !val) trackColor = ImVec4(0.72f, 0.75f, 0.80f, 1.00f);
    drawList->AddRectFilled(startPos, ImVec2(startPos.x + width, startPos.y + height),
        ImGui::ColorConvertFloat4ToU32(trackColor), height * 0.5f);
    const float knobX = val ? startPos.x + width - knobPadding - knobSize : startPos.x + knobPadding;
    drawList->AddCircleFilled(ImVec2(knobX + (knobSize * 0.5f), startPos.y + (height * 0.5f)),
        knobSize * 0.5f, ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)));
    if (!visibleLabel.empty()) {
        ImGui::SameLine(0, 10.0f);
        ImGui::AlignTextToFramePadding();
        const std::string terminated(visibleLabel);
        ImGui::TextUnformatted(terminated.c_str(), nullptr);
    }
    return changed;
}
bool uiImButton(const char* text, vec2 size) {
    auto* style = &ImGui::GetStyle();
    const ImVec2 originalFramePadding = style->FramePadding;
    if (size.x != 0) style->FramePadding.x = 0;
    if (size.y != 0) style->FramePadding.y = 0;
    auto* col = uiGetColor(UIColor::ButtonTextColor);
    if (col) ImGui::PushStyleColor(ImGuiCol_Text, *col);
    const bool result = ImGui::Button(text, ImVec2(size.x, size.y));
    if (col) ImGui::PopStyleColor();
    style->FramePadding = originalFramePadding;
    return result;
}
void uiImRange(float& cmin, float& cmax, float min, float max) { ImGui::DragFloatRange2("", &cmin, &cmax, 1.0f, min, max); }
void uiImRange(int& cmin, int& cmax, int min, int max) { ImGui::DragIntRange2("", &cmin, &cmax, 1.0f, min, max); }
void uiImDrag(float& value, float min, float max) { ImGui::DragFloat("", &value, 1.0f, min, max); }
void uiImDrag(int& value, int min, int max) { ImGui::DragInt("", &value, 1.0f, min, max); }
void uiImDrag2(std::span<float> value, float min, float max) { ImGui::DragFloat2("", value.data(), 1.0f, min, max); }
void uiImDrag3(std::span<float> value, float min, float max) { ImGui::DragFloat3("", value.data(), 1.0f, min, max); }
void uiImDrag4(std::span<float> value, float min, float max) { ImGui::DragFloat4("", value.data(), 1.0f, min, max); }
bool uiImBeginComboBox(std::string_view id, const char* previewName) {
    ImGui::PushID(id.data(), id.data() + id.size());
    const bool ret = ImGui::BeginCombo("###COMBO", previewName);
    if (!ret) ImGui::PopID();
    return ret;
}
void uiImEndComboBox() { ImGui::EndCombo(); ImGui::PopID(); }
bool uiImSelectable(const char* name, bool selected) { return ImGui::Selectable(name, selected); }
bool uiImColor4(const char* label, float (*colors)[4]) { return ImGui::ColorEdit4(label, *colors); }
bool uiImColorButton4(const char* label, float (*colors)[4]) { return ImGui::ColorEdit4(label, *colors, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar); }
bool uiImColorPicker4(const char* label, float (*colors)[4]) { return ImGui::ColorPicker4(label, *colors); }
bool uiImColorSwatch4(const char* description, vec4 color) { return ImGui::ColorButton(description, ImVec4(color.x, color.y, color.z, color.w)); }
bool uiImColor3(const char* label, float (*colors)[3]) { return ImGui::ColorEdit3(label, *colors); }
bool uiImColorButton3(const char* label, float (*colors)[3]) { return ImGui::ColorEdit3(label, *colors, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar); }
bool uiImColorPicker3(const char* label, float (*colors)[3]) { return ImGui::ColorPicker3(label, *colors); }
bool uiImColorSwatch3(const char* description, vec3 color) {
    // The D aggregate leaves the fourth float at float.init (NaN), not 1.
    return ImGui::ColorButton(description, ImVec4(color.x, color.y, color.z, std::numeric_limits<float>::quiet_NaN()));
}
}
