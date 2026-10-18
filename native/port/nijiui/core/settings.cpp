// nijiui/core/settings.d control flow, with the authorized Boost.JSON backend.
#include "nijiui/core/settings.hpp"
#include "nijiui/core/path.hpp"
#include "dcompat/string.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
namespace nijiui {
namespace {
thread_local nijiexpose::json::Value settings = nijiexpose::json::Object{};
// A loaded scalar/array remains a leaf, as in the source FghjNode. Assigning
// children does not replace that leaf; serialization still writes the leaf.
thread_local nijiexpose::json::Object leafChildren;
std::filesystem::path path(std::string_view value) { return nijiexpose::dcompat::toUTF16z(value); }
}
std::string inSettingsPath() {
    return nijiexpose::dcompat::toUTF8((path(inGetAppConfigPath()) / L"settings.json").native());
}
void inSettingsLoad() {
    settings = nijiexpose::json::Object{};
    leafChildren.clear();
    if (std::filesystem::exists(path(inSettingsPath()))) {
        std::ifstream input(path(inSettingsPath()), std::ios::binary);
        if (!input) throw std::runtime_error("Unable to read settings");
        const std::string text((std::istreambuf_iterator<char>(input)), {});
        if (input.bad()) throw std::runtime_error("Unable to read settings");
        settings = nijiexpose::json::parse(text);
    }
}
void inSettingsSave() {
    const auto filename = path(inSettingsPath());
    const auto text = nijiexpose::json::serialize(settings);
    std::ofstream output(filename, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("Unable to write settings");
    output << text;
    output.close();
    if (!output) throw std::runtime_error("Unable to write settings");
}
bool inSettingsCanGet(std::string_view name) {
    return settings.is_object() ? settings.as_object().contains(name) : leafChildren.contains(name);
}
const nijiexpose::json::Value& inSettingsGetJson(std::string_view name) {
    return nijiexpose::json::require(settings, name);
}
void inSettingsSetJson(std::string_view name, nijiexpose::json::Value value) {
    if (settings.is_object()) settings.as_object()[name] = std::move(value);
    else leafChildren[name] = std::move(value);
}
}
