// Port of nijiui/core/window/package.d.
#include "window.hpp"
#include <algorithm>
namespace nijiui {
namespace { thread_local std::vector<std::shared_ptr<InWindow>> subwindows; }
std::vector<std::shared_ptr<InWindow>>& inWindowListGet() { return subwindows; }
void inWindowListAdd(std::shared_ptr<InWindow> window) { subwindows.push_back(std::move(window)); }
void inWindowListRemove(const std::shared_ptr<InWindow>& window) {
    if(subwindows.empty()) return;
    auto found=std::find(subwindows.begin(),subwindows.end(),window);
    if(found!=subwindows.end()) subwindows.erase(found);
}
}
