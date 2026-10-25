#pragma once
#include "nijiui/core/window/appwin.hpp"
namespace nijiexpose::windows {
std::shared_ptr<nijiui::InApplicationWindow> neCreateWindow(const std::vector<std::string>& args);
void neWindowSetThrottlingRate(int rate);
}
