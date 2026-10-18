// Color storage functions from nijiui/widgets/input.d (BSD-2-Clause).
#include "nijiui/widgets/colors.hpp"
#include <unordered_map>
namespace nijiui {
namespace { thread_local std::unordered_map<UIColor, ImVec4> colors; }
ImVec4* uiGetColor(UIColor id) {
    if (colors.contains(id)) return &colors[id];
    return nullptr;
}
void uiSetColor(UIColor id, ImVec4 value) { colors[id] = value; }
void uiDeleteColor(UIColor id) {
    if (colors.contains(id)) colors.erase(id);
}
}
