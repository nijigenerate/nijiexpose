#pragma once
#include "nijiui/widgets/basic.hpp"
#include "nijiui/widgets/colors.hpp"
#include <span>
#include <string>
namespace nijiui {
bool uiImInputText(std::string_view id, std::string& buffer, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
bool uiImInputText(std::string_view id, float width, std::string& buffer, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
bool uiImInputText(std::string_view id, std::string_view label, std::string& buffer, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
bool uiImInputText(std::string_view id, std::string_view label, float width, std::string& buffer, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
bool uiImCheckbox(const char* text, bool& value);
bool uiImButton(const char* text, vec2 size = {0, 0});
void uiImRange(float& cmin, float& cmax, float min, float max);
void uiImRange(int& cmin, int& cmax, int min, int max);
void uiImDrag(float& value, float min, float max);
void uiImDrag(int& value, int min, int max);
void uiImDrag2(std::span<float> value, float min, float max);
void uiImDrag3(std::span<float> value, float min, float max);
void uiImDrag4(std::span<float> value, float min, float max);
bool uiImBeginComboBox(std::string_view id, const char* previewName);
void uiImEndComboBox();
bool uiImSelectable(const char* name, bool selected = false);
bool uiImColor4(const char* label, float (*colors)[4]);
bool uiImColorButton4(const char* label, float (*colors)[4]);
bool uiImColorPicker4(const char* label, float (*colors)[4]);
bool uiImColorSwatch4(const char* description, vec4 color);
bool uiImColor3(const char* label, float (*colors)[3]);
bool uiImColorButton3(const char* label, float (*colors)[3]);
bool uiImColorPicker3(const char* label, float (*colors)[3]);
bool uiImColorSwatch3(const char* description, vec3 color);
}
