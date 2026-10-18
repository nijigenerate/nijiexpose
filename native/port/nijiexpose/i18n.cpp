// source/nijiexpose/i18n.d, with authorized GNU gettext/Boost.JSON backends.
#include "nijiexpose/i18n.hpp"
#include "i18n/tr.hpp"
#include "nijiui/core/path.hpp"
#include "nijiui/core/settings.hpp"
#include "dcompat/string.hpp"
#include <algorithm>
#include <filesystem>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace nijiexpose {
namespace {
thread_local std::vector<TLEntry> localeFiles;
[[maybe_unused]] std::string neLocaleLanguageNameMarker() { return i18n::_("LANG_NAME"); }
std::filesystem::path path(std::string_view value) { return dcompat::toUTF16z(value); }
std::string neGetCultureExpression(std::string_view langcode) {
    for (const auto& locale : localeFiles) if (locale.code == langcode) return locale.humanName;
    if (langcode.size() >= 5) return i18n::i18nGetCultureLanguage(langcode) + " (" +
        (langcode == "zh-CN" ? "Simplified" : langcode == "zh-TW" ? "Traditional" : i18n::i18nGetCultureCountry(langcode)) + ")";
    return i18n::i18nGetCultureLanguage(langcode);
}
void neLocaleScan(const std::filesystem::path& directory) {
    if (!std::filesystem::exists(directory)) return;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        auto extension = entry.path().extension().native();
        for (auto& c : extension) if (c >= L'A' && c <= L'Z') c += L'a' - L'A';
        if (extension != L".mo") continue;
        const auto langcode = dcompat::toUTF8(entry.path().stem().native());
        if (!i18n::i18nValidateCultureCode(langcode)) continue;
        const auto filename = dcompat::toUTF8(entry.path().native());
        auto langName = i18n::i18nGetLanguageName(filename);
        if (langName == "<UNKNOWN LANGUAGE>") langName = neGetCultureExpression(langcode);
        bool found = false;
        for (const auto& locale : localeFiles) if (locale.code == langcode) { found = true; break; }
        if (found) continue;
        localeFiles.push_back({langName, langcode, filename, dcompat::toUTF8(directory.native())});
    }
}
std::filesystem::path executableDirectory() {
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), DWORD(buffer.size()));
    if (!length || length >= buffer.size()) throw std::runtime_error("Unable to get executable path");
    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path();
}
}
void neLocaleInit() {
    localeFiles.clear();
    neLocaleScan(path(nijiui::inGetAppLocalePath()));
    neLocaleScan(executableDirectory());
    neLocaleScan(executableDirectory() / L"i18n");
    std::sort(localeFiles.begin(), localeFiles.end(), neCompareLocaleEntries);
    neMarkLocaleDups(localeFiles);
}
void neLocaleInitFromSettings() {
    neLocaleInit();
    if (nijiui::inSettingsCanGet("lang")) {
        const auto lang = nijiui::inSettingsGet<std::string>("lang");
        if (const auto* entry = neLocaleGetEntryFor(lang)) i18n::i18nLoadLanguage(entry->file);
    }
}
bool neCompareLocaleEntries(const TLEntry& a, const TLEntry& b) {
    const auto left = dcompat::toUTF16z(a.humanName), right = dcompat::toUTF16z(b.humanName);
    const int cmp = CompareStringOrdinal(left.data(), int(left.size()), right.data(), int(right.size()), TRUE);
    if (cmp == CSTR_EQUAL) return a.path < b.path;
    return cmp == CSTR_LESS_THAN;
}
void neMarkLocaleDups(std::span<TLEntry> entries) {
    if (entries.size() <= 1) return;
    auto* prevEntry = &entries[0];
    bool prevIsDup = false;
    for (auto& entry : entries.subspan(1)) {
        const bool entryIsDup = entry.humanName == prevEntry->humanName;
        if (prevIsDup || entryIsDup) prevEntry->humanName += " (" + prevEntry->code + ")";
        prevIsDup = entryIsDup;
        prevEntry = &entry;
    }
    if (prevIsDup) prevEntry->humanName += " (" + prevEntry->code + ")";
}
std::string neLocaleCurrentCode() { return nijiui::inSettingsGet<std::string>("lang", "en"); }
std::string neLocaleCurrentName() {
    const auto code = neLocaleCurrentCode();
    return neGetCultureExpression(code.empty() ? "en" : code);
}
void neLocaleSet(std::string code) {
    nijiui::inSettingsSet("lang", code);
    if (code.empty() || code == "en") { i18n::i18nClearLanguage(); return; }
    if (const auto* entry = neLocaleGetEntryFor(code)) i18n::i18nLoadLanguage(entry->file);
}
TLEntry* neLocaleGetEntryFor(std::string_view code) {
    for (auto& entry : localeFiles) if (entry.code == code) return &entry;
    return nullptr;
}
std::span<TLEntry> neLocaleGetEntries() { return localeFiles; }
}
