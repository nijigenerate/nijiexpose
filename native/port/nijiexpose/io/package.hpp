#pragma once
#include <string>
#include <vector>
#include <span>
namespace nijiexpose::io {
struct TFD_Filter {std::vector<std::string> patterns;std::string description;};
std::wstring buildFilterString(std::span<const TFD_Filter> filters);
std::string insShowOpenDialog(std::span<const TFD_Filter> filters,const std::string& title="Open...",const std::string& parentWindow="");
}
