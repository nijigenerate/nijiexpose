#pragma once
#include "nijiui/input.hpp"
#include <limits>
#include <string_view>
namespace nijiui {
void uiImDummy(vec2 size);
void uiImSpacer(vec2 size);
void uiImSameLine(float startOffset = 0, float spacing = -1);
vec2 uiImAvailableSpace();
vec2 uiImMeasureString(std::string_view text);
void uiImLabel(std::string_view text);
void uiImLabelColored(std::string_view text, vec4 color);
void uiImLabelDisabled(std::string_view text);
void uiImLabelWrapped(std::string_view text, float wrapPosition = 0);
void uiImPushTextWrapPos(float wrapPosition = 0);
void uiImPopTextWrapPos();
void uiImTooltip(std::string_view tip);
bool uiImBeginMainMenuBar();
void uiImEndMainMenuBar();
bool uiImBeginMenuBar();
void uiImEndMenuBar();
bool uiImBeginMenu(const char* label, bool enabled = true);
void uiImEndMenu();
bool uiImMenuItem(const char* label, const char* shortcut = "", bool selected = false, bool enabled = true);
bool uiImBeginPopup(std::string_view id);
void uiImEndPopup();
void uiImOpenPopup(std::string_view id);
void uiImRightClickPopup(std::string_view id);
void uiImPush(int id);
void uiImPush(std::string_view id);
template<typename T> void uiImPush(T* id) { ImGui::PushID(static_cast<const void*>(id)); }
void uiImPop();
void uiImIndent(float indentW = 0);
void uiImUnindent();
void uiImSeperator();
void uiImNewLine();
bool uiImHeader(const char* label, bool defaultOpen = false);
bool uiImBeginChild(std::string_view id, vec2 size = {0, 0}, bool border = false);
void uiImEndChild();
void uiImProgress(float progress, vec2 size = {-std::numeric_limits<float>::min(), 0}, const char* text = nullptr);
}
