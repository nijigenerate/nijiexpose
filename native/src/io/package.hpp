#pragma once
#include <string>
#include <vector>
#include <span>
struct SDL_Window;
namespace nijiexpose::io {
struct TFD_Filter {std::vector<std::string> patterns;std::string description;};
std::string insShowOpenDialog(std::span<const TFD_Filter> filters,const std::string& title="Open...",SDL_Window* parentWindow=nullptr);
}
