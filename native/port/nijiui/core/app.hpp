#pragma once
#include <string>
namespace nijiui {
struct InApplication {
    std::string name;
    std::string configDirName;
    std::string humanName;
};
void inSetApplication(InApplication app);
InApplication& inGetApplication();
}
