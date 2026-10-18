#pragma once
#include "nijiexpose/windows/main.hpp"
namespace nijiexpose {
std::shared_ptr<nijiui::InApplicationWindow> initializeApplication(const std::vector<std::string>& args);
void shutdownApplication();
}
