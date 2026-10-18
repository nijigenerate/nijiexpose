// GNU gettext adapter. This backend replacement is explicitly authorized.
// Flat D-era .mo catalogs are staged into gettext's normal domain layout.
#include "i18n/tr.hpp"
#include "dcompat/string.hpp"
#include <libintl.h>
#include <clocale>
#include <cstdlib>
#include <filesystem>
#include <list>
#include <cstring>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace i18n {
namespace {
using nijiexpose::dcompat::toUTF8;
using nijiexpose::dcompat::toUTF16z;
std::filesystem::path path(std::string_view value) { return toUTF16z(value); }
std::wstring localeName(std::string_view code) {
    auto name = toUTF16z(code);
    for (auto& c : name) if (c == L'_') c = L'-';
    return name;
}
std::string localeInfo(std::string_view code, LCTYPE type) {
    const auto name = localeName(code);
    const int size = GetLocaleInfoEx(name.c_str(), type, nullptr, 0);
    if (!size) return {};
    std::wstring result(size, L'\0');
    GetLocaleInfoEx(name.c_str(), type, result.data(), size);
    result.resize(size - 1);
    return toUTF8(result);
}
struct Catalog { std::string domain; std::string language; };
struct Catalogs {
    std::filesystem::path root;
    std::size_t sequence = 0;
    Catalog loaded;
    std::list<std::string> cStrings;
    Catalogs() {
        // GNU gettext needs an initialized non-C LC_CTYPE locale. Keep numeric
        // formatting untouched; Boost.JSON is independently locale invariant.
        if (!std::setlocale(LC_CTYPE, ".UTF8")) throw std::runtime_error("Unable to initialize gettext locale");
        wchar_t temporaryPath[MAX_PATH + 1];
        wchar_t temporaryName[MAX_PATH + 1];
        if (!GetTempPathW(MAX_PATH + 1, temporaryPath) || !GetTempFileNameW(temporaryPath, L"nex", 0, temporaryName))
            throw std::runtime_error("Unable to create gettext catalog cache");
        root = temporaryName;
        std::filesystem::remove(root);
        std::filesystem::create_directory(root);
    }
    ~Catalogs() {
        std::error_code error;
        // Only this instance's generated temporary directory is owned here.
        if (!root.empty()) std::filesystem::remove_all(root, error);
    }
    Catalog stage(std::string_view filename) {
        const auto source = path(filename);
        if (!std::filesystem::is_regular_file(source)) throw std::runtime_error("Translation catalog does not exist");
        Catalog catalog{"nijiexpose_" + std::to_string(++sequence), toUTF8(source.stem().native())};
        for (auto& c : catalog.language) if (c == '-') c = '_';
        // LANG_NAME metadata need not be tied to a valid Windows locale.
        if (catalog.language.empty()) catalog.language = "en";
        const auto destination = root / path(catalog.language) / L"LC_MESSAGES";
        std::filesystem::create_directories(destination);
        std::filesystem::copy_file(source, destination / path(catalog.domain + ".mo"));
        if (!wbindtextdomain(catalog.domain.c_str(), root.c_str()) ||
            !bind_textdomain_codeset(catalog.domain.c_str(), "UTF-8"))
            throw std::runtime_error("Unable to bind gettext catalog");
        return catalog;
    }
    void activate(const Catalog& catalog) {
        auto setEnvironment = [](const char* name, const wchar_t* wideName, const std::string& value) {
            if (_putenv_s(name, value.c_str()) != 0) throw std::runtime_error("Unable to select gettext language");
            const auto wide = toUTF16z(value);
            if (!SetEnvironmentVariableW(wideName, wide.empty() ? nullptr : wide.c_str()))
                throw std::runtime_error("Unable to select gettext language");
        };
        // libintl on Windows also consults these variables directly. Its C
        // locale suppression precedes LANGUAGE, even after setlocale(). An
        // explicit in-app selection therefore overrides launcher C/POSIX.
        if (const auto* all = std::getenv("LC_ALL"); all && (std::strcmp(all, "C") == 0 || std::strcmp(all, "POSIX") == 0))
            setEnvironment("LC_ALL", L"LC_ALL", "");
        setEnvironment("LC_MESSAGES", L"LC_MESSAGES", catalog.language.empty() ? "C" : catalog.language);
        // Explicit app selection must work even when the launcher sets LANG=C.
        if (!std::setlocale(LC_MESSAGES, catalog.language.empty() ? "C" : catalog.language.c_str()))
            throw std::runtime_error("Unable to select gettext message locale");
        setEnvironment("LANGUAGE", L"LANGUAGE", catalog.language);
        textdomain(catalog.domain.empty() ? "nijiexpose" : catalog.domain.c_str());
    }
};
Catalogs& catalogs() { static Catalogs state; return state; }
}
bool i18nValidateCultureCode(std::string_view code) {
    if (code == "C") return true;
    if (code.size() < 2) return false;
    return !localeInfo(code, LOCALE_SISO639LANGNAME).empty();
}
std::string i18nGetCultureLanguage(std::string_view code) { return localeInfo(code, LOCALE_SENGLISHLANGUAGENAME); }
std::string i18nGetCultureNativeLanguageEstimate(std::string_view code) { return localeInfo(code, LOCALE_SNATIVELANGUAGENAME); }
std::string i18nGetCultureCountry(std::string_view code) { return localeInfo(code, LOCALE_SENGLISHCOUNTRYNAME); }
std::string i18nGetLanguageName(std::string_view file) {
    auto& state = catalogs();
    const auto temporary = state.stage(file);
    state.activate(temporary);
    std::string result(dgettext(temporary.domain.c_str(), "LANG_NAME"));
    state.activate(state.loaded);
    const auto code = toUTF8(path(file).stem().native());
    if (result == "LANG_NAME") result = i18nGetCultureNativeLanguageEstimate(code);
    if (result == "LANG_NAME") result = i18nGetCultureLanguage(code);
    if (result.empty()) return "<UNKNOWN LANGUAGE>";
    return result;
}
void i18nClearLanguage() {
    auto& state = catalogs();
    state.loaded = {};
    state.activate(state.loaded);
}
bool i18nLoadLanguage(std::string_view file) {
    if (file.empty()) { i18nClearLanguage(); return true; }
    auto extension = path(file).extension().native();
    for (auto& c : extension) if (c >= L'A' && c <= L'Z') c += L'a' - L'A';
    if (extension != L".mo") throw std::runtime_error("i18n: Invalid file format " + toUTF8(path(file).extension().native()));
    try {
        auto& state = catalogs();
        auto catalog = state.stage(file);
        state.activate(catalog);
        state.loaded = std::move(catalog);
    } catch (const std::exception&) { return false; }
    return true;
}
std::string _(std::string_view text) {
    auto& state = catalogs();
    if (state.loaded.domain.empty()) return std::string(text);
    return dgettext(state.loaded.domain.c_str(), std::string(text).c_str());
}
const char* __(std::string_view text) {
    auto& state = catalogs();
    state.cStrings.emplace_back(_(text));
    return state.cStrings.back().c_str();
}
}
