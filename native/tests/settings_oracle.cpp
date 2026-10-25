#include "nijiui/core/app.hpp"
#include "nijiui/core/settings.hpp"
#include <bit>
#include <cstdio>
void emit(const std::string& value) { for (unsigned char ch : value) std::printf("%02x", ch); }
int main() {
    using namespace nijiui;
    inSetApplication({"net.nijilive.nijiexpose", "nijiexpose", "nijiexpose"});
    try {
        inSettingsLoad();
        const auto scale = inSettingsGet<float>("UIScale", 1.0f);
        const auto language = inSettingsGet<std::optional<std::string>>("lang");
        const auto color = inSettingsGet<std::array<float, 4>>("bgColor", {0.5f, 0.5f, 0.5f, 0});
        std::printf("%d %d %08x %d ", inSettingsCanGet("UIScale"), inSettingsGet<bool>("visible", true),
            std::bit_cast<unsigned>(scale), bool(language));
        if (language) emit(*language);
        std::printf(" ");
        for (auto item : color) std::printf("%08x ", std::bit_cast<unsigned>(item));
        std::puts("");
        inSettingsSet("new", 42);
        inSettingsSet("lang", std::string("ja"));
        inSettingsSave();
        inSettingsLoad();
        std::printf("%d ", inSettingsGet<int>("new"));
        emit(inSettingsGet<std::string>("lang"));
        std::puts("");
    } catch (const std::exception&) { std::puts("Exception"); }
}
