#include "nijiui/core/app.hpp"
#include "nijiui/core/path.hpp"
#include <iostream>

int main() {
    nijiui::inSetApplication({"net.nijilive.nijiexpose", "nijiexpose", "nijiexpose"});
    std::cout << nijiui::inGetAppConfigPath();
}
