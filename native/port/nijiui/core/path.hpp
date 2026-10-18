#pragma once
#include <string>
namespace nijiui {
inline constexpr char ENV_CONFIG_PATH[] = "IN_CONFIG_PATH";
std::string inGetAppConfigPath();
std::string inGetAppImguiConfigFile();
std::string inGetAppFontsPath();
std::string inGetAppLocalePath();
std::string inGetAppCustomPath(std::string name);
}
