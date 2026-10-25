#pragma once
#include <imgui.h>
namespace nijiui {
enum class UIColor { ButtonTextColor };
ImVec4* uiGetColor(UIColor id);
void uiSetColor(UIColor id, ImVec4 value);
void uiDeleteColor(UIColor id);
}
