#include "nijiui/core/app.hpp"
#include "nijiui/core/path.hpp"
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int main() {
    using namespace nijiui;
    inSetApplication({"net.nijilive.nijiexpose", "nijiexpose", "nijiexpose"});
    try {
        std::cout << inGetAppConfigPath() << '\n';
        std::cout << inGetAppImguiConfigFile() << '\n';
        std::cout << inGetAppFontsPath() << '\n';
        std::cout << inGetAppLocalePath() << '\n';
        std::cout << inGetAppCustomPath("plugins") << '\n';
        SetEnvironmentVariableW(L"IN_CONFIG_PATH", L"changed-after-cache");
        std::cout << inGetAppConfigPath() << '\n';
    } catch (const std::exception&) { std::cout << "ERROR\n"; }
}
