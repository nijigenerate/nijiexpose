#pragma once
#include "nijiui/widgets/basic.hpp"
#include <imgui.h>
namespace nijiui {
enum class UiImCategoryFlags { None = 0, NoCollapse = 1, DefaultClosed = 2 };
constexpr UiImCategoryFlags operator&(UiImCategoryFlags a, UiImCategoryFlags b) {
    return static_cast<UiImCategoryFlags>(static_cast<int>(a) & static_cast<int>(b));
}
constexpr UiImCategoryFlags operator|(UiImCategoryFlags a, UiImCategoryFlags b) {
    return static_cast<UiImCategoryFlags>(static_cast<int>(a) | static_cast<int>(b));
}
bool uiImBeginCategory(const char* title, UiImCategoryFlags flags = UiImCategoryFlags::None,
    void (*callback)(float, float) = nullptr);
bool uiImBeginCategory(const char* title, ImVec4 color, UiImCategoryFlags flags = UiImCategoryFlags::None,
    void (*callback)(float, float) = nullptr);
void uiImEndCategory();
}
