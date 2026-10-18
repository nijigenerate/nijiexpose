// Windows branch of nijiui/core/path.d. Retain cache and scope(success) order.
#include "nijiui/core/path.hpp"
#include "nijiui/core/app.hpp"
#include "dcompat/string.hpp"
#include "dcompat/scope.hpp"
#include <filesystem>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace nijiui {
namespace {
thread_local std::string cachedConfigDir;
thread_local std::string cachedImguiFileDir;
thread_local std::string cachedFontDir;
thread_local std::string cachedLocaleDir;
thread_local std::string inForcedConfigDir;

std::string environmentGet(const wchar_t* name) {
    const DWORD length = GetEnvironmentVariableW(name, nullptr, 0);
    if (!length) return {};
    std::wstring result(length, L'\0');
    GetEnvironmentVariableW(name, result.data(), length);
    result.resize(length - 1);
    return nijiexpose::dcompat::toUTF8(result);
}
std::filesystem::path nativePath(const std::string& value) { return nijiexpose::dcompat::toUTF16z(value); }
std::string buildPath(const std::string& left, const std::string& right) {
    return nijiexpose::dcompat::toUTF8((nativePath(left) / nativePath(right)).native());
}
bool exists(const std::string& value) { return std::filesystem::exists(nativePath(value)); }
void mkdirRecurse(const std::string& value) { std::filesystem::create_directories(nativePath(value)); }
}
std::string inGetAppConfigPath() {
    if (!cachedConfigDir.empty()) return cachedConfigDir;
    if (!inForcedConfigDir.empty()) return inForcedConfigDir;
    std::string appDataDir;
    nijiexpose::dcompat::ScopeSuccess complete([&] {
        if (!inForcedConfigDir.empty()) {
            if (!exists(inForcedConfigDir)) mkdirRecurse(inForcedConfigDir);
        } else {
            cachedConfigDir = appDataDir;
            if (!exists(cachedConfigDir)) mkdirRecurse(cachedConfigDir);
        }
    });
    appDataDir = environmentGet(L"AppData");
    inForcedConfigDir = environmentGet(L"IN_CONFIG_PATH");
    if (!inForcedConfigDir.empty()) return inForcedConfigDir;
    if (appDataDir.empty()) appDataDir = nijiexpose::dcompat::toUTF8(std::filesystem::current_path().native());
    appDataDir = buildPath(appDataDir, "." + inGetApplication().configDirName);
    // D returns a copied string slice; keep the local intact for scope(success).
    return std::string(appDataDir);
}
std::string inGetAppImguiConfigFile() {
    if (!cachedImguiFileDir.empty()) return cachedImguiFileDir;
    cachedImguiFileDir = buildPath(inGetAppConfigPath(), "imgui.ini");
    return cachedImguiFileDir;
}
std::string inGetAppFontsPath() {
    if (!cachedFontDir.empty()) return cachedFontDir;
    cachedFontDir = buildPath(inGetAppConfigPath(), "fonts");
    if (!exists(cachedFontDir)) mkdirRecurse(cachedFontDir);
    return cachedFontDir;
}
std::string inGetAppLocalePath() {
    if (!cachedLocaleDir.empty()) return cachedLocaleDir;
    cachedLocaleDir = buildPath(inGetAppConfigPath(), "i18n");
    if (!exists(cachedLocaleDir)) mkdirRecurse(cachedLocaleDir);
    return cachedLocaleDir;
}
std::string inGetAppCustomPath(std::string name) {
    const auto path = buildPath(inGetAppConfigPath(), name);
    if (!exists(path)) mkdirRecurse(path);
    return path;
}
}
