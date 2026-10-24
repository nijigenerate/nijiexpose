#pragma once
#include <string>
#include <string_view>
namespace nijiexpose::utils {
std::string launchPath(std::string_view input);
bool executableFileAvailable(std::string_view input);
}
