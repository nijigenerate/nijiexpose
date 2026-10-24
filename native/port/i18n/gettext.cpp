// GNU gettext adapter. This backend replacement is explicitly authorized.
// Flat D-era .mo catalogs are staged into gettext's normal domain layout.
#include "i18n/tr.hpp"
#include "dcompat/string.hpp"
#include <libintl.h>
#include <clocale>
#include <cstdlib>
#include <filesystem>
#include <unordered_set>
#include <cstring>
#include <stdexcept>
#include <array>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif
#endif
namespace i18n {
namespace {
using nijiexpose::dcompat::toUTF8;
using nijiexpose::dcompat::toUTF16z;
std::filesystem::path path(std::string_view value) { return nijiexpose::dcompat::pathFromUTF8(value); }
#ifdef _WIN32
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
#else
std::string systemLocaleName() {
#ifdef __APPLE__
    auto locale=CFLocaleCopyCurrent();
    if(!locale)return "en_US.UTF-8";
    auto identifier=CFLocaleGetIdentifier(locale);
    const auto capacity=CFStringGetMaximumSizeForEncoding(CFStringGetLength(identifier),kCFStringEncodingUTF8)+1;
    std::vector<char> buffer(static_cast<std::size_t>(capacity));
    std::string result="en_US.UTF-8";
    if(CFStringGetCString(identifier,buffer.data(),capacity,kCFStringEncodingUTF8)) {
        result=buffer.data();
        if(result.find('.')==std::string::npos)result += ".UTF-8";
    }
    CFRelease(locale);
    return result;
#else
    return {};
#endif
}
std::string localeInfo(std::string_view code, bool country=false) {
#ifdef __APPLE__
    auto identifier=CFStringCreateWithBytes(nullptr,reinterpret_cast<const UInt8*>(code.data()),code.size(),kCFStringEncodingUTF8,false);
    if(!identifier)return {};
    auto canonical=CFLocaleCreateCanonicalLocaleIdentifierFromString(nullptr,identifier);
    auto key=country?kCFLocaleCountryCode:kCFLocaleLanguageCode;
    auto sourceLocale=CFLocaleCreate(nullptr,canonical);
    auto value=sourceLocale?static_cast<CFStringRef>(CFLocaleGetValue(sourceLocale,key)):nullptr;
    auto displayLocale=CFLocaleCopyCurrent();
    auto display=value?CFLocaleCopyDisplayNameForPropertyValue(displayLocale,country?kCFLocaleCountryCode:kCFLocaleLanguageCode,value):nullptr;
    std::string result;
    if(display){
        const auto length=CFStringGetMaximumSizeForEncoding(CFStringGetLength(display),kCFStringEncodingUTF8)+1;
        std::vector<char> buffer(static_cast<std::size_t>(length));
        if(CFStringGetCString(display,buffer.data(),length,kCFStringEncodingUTF8))result=buffer.data();
        CFRelease(display);
    }
    if(sourceLocale)CFRelease(sourceLocale); if(displayLocale)CFRelease(displayLocale); if(canonical)CFRelease(canonical); CFRelease(identifier);
    return result;
#else
    (void)country; return std::string(code);
#endif
}
#endif
struct Catalog { std::string domain; std::string language; };
struct Catalogs {
    std::filesystem::path root;
    std::size_t sequence = 0;
    Catalog loaded;
    std::unordered_set<std::string> cStrings;
    Catalogs() {
        // GNU gettext needs an initialized non-C LC_CTYPE locale. Keep numeric
        // formatting untouched; Boost.JSON is independently locale invariant.
        if (!std::setlocale(LC_CTYPE,
#ifdef _WIN32
            ".UTF8"
#else
            ""
#endif
            )) throw std::runtime_error("Unable to initialize gettext locale");
#ifdef _WIN32
        wchar_t temporaryPath[MAX_PATH + 1];
        wchar_t temporaryName[MAX_PATH + 1];
        if (!GetTempPathW(MAX_PATH + 1, temporaryPath) || !GetTempFileNameW(temporaryPath, L"nex", 0, temporaryName))
            throw std::runtime_error("Unable to create gettext catalog cache");
        root = temporaryName;
        std::filesystem::remove(root);
        std::filesystem::create_directory(root);
#else
        root=std::filesystem::temp_directory_path()/std::filesystem::path("nijiexpose-gettext-"+std::to_string(static_cast<long long>(getpid())));
        std::filesystem::create_directories(root);
#endif
    }
    ~Catalogs() {
        std::error_code error;
        // Only this instance's generated temporary directory is owned here.
        if (!root.empty()) std::filesystem::remove_all(root, error);
    }
    Catalog stage(std::string_view filename) {
        const auto source = path(filename);
        if (!std::filesystem::is_regular_file(source)) throw std::runtime_error("Translation catalog does not exist");
        Catalog catalog{"nijiexpose_" + std::to_string(++sequence), nijiexpose::dcompat::pathToUTF8(source.stem())};
        for (auto& c : catalog.language) if (c == '-') c = '_';
        // LANG_NAME metadata need not be tied to a valid Windows locale.
        if (catalog.language.empty()) catalog.language = "en";
        const auto destination = root / path(catalog.language) / L"LC_MESSAGES";
        std::filesystem::create_directories(destination);
        std::filesystem::copy_file(source, destination / path(catalog.domain + ".mo"));
#ifdef _WIN32
        if (!wbindtextdomain(catalog.domain.c_str(), root.c_str()) ||
#else
        if (!bindtextdomain(catalog.domain.c_str(), root.c_str()) ||
#endif
            !bind_textdomain_codeset(catalog.domain.c_str(), "UTF-8"))
            throw std::runtime_error("Unable to bind gettext catalog");
        return catalog;
    }
    void activate(const Catalog& catalog) {
        auto setEnvironment = [](const char* name, const wchar_t* wideName, const std::string& value) {
#ifdef _WIN32
            if (_putenv_s(name, value.c_str()) != 0) throw std::runtime_error("Unable to select gettext language");
            const auto wide = toUTF16z(value);
            if (!SetEnvironmentVariableW(wideName, wide.empty() ? nullptr : wide.c_str()))
                throw std::runtime_error("Unable to select gettext language");
#else
            (void)wideName;
            if (value.empty()) unsetenv(name);
            else if (setenv(name,value.c_str(),1)!=0) throw std::runtime_error("Unable to select gettext language");
#endif
        };
        // libintl on Windows also consults these variables directly. Its C
        // locale suppression precedes LANGUAGE, even after setlocale(). An
        // explicit in-app selection therefore overrides launcher C/POSIX.
#ifdef _WIN32
        if (const auto* all = std::getenv("LC_ALL"); all &&
            (std::strcmp(all, "C") == 0 || std::strcmp(all, "POSIX") == 0 ||
             std::string_view(all).starts_with("C.") || std::string_view(all).starts_with("POSIX.")))
            setEnvironment("LC_ALL", L"LC_ALL", "");
#else
        if (const auto* all=std::getenv("LC_ALL"); all && (std::strcmp(all,"C")==0||std::strcmp(all,"POSIX")==0||std::string_view(all).starts_with("C.")||std::string_view(all).starts_with("POSIX."))) setEnvironment("LC_ALL",nullptr,"");
#endif
#ifdef _WIN32
        setEnvironment("LC_MESSAGES", L"LC_MESSAGES", catalog.language.empty() ? "C" : catalog.language);
#else
        // macOS locale identifiers differ from bare catalog language codes.
        // Let the user's installed locale drive LC_MESSAGES and let GNU
        // gettext's LANGUAGE override select the in-app translation.
        setEnvironment("LC_MESSAGES", nullptr, systemLocaleName());
        if (!std::setlocale(LC_MESSAGES, "")) throw std::runtime_error("Unable to select gettext message locale");
        if (!catalog.language.empty()) {
            const std::string current=std::setlocale(LC_MESSAGES,nullptr);
            if (current=="C" || current=="POSIX" || current.starts_with("C.") || current.starts_with("POSIX.")) {
                bool selected=false;
                // glibc also suppresses LANGUAGE in C.UTF-8. Use a real
                // installed message locale; keep LC_NUMERIC untouched.
                for (const auto& fallback : {catalog.language+".UTF-8",std::string("en_US.UTF-8"),std::string("en_US.utf8")}) {
                    if (std::setlocale(LC_MESSAGES,fallback.c_str())) {
                        setEnvironment("LC_MESSAGES",nullptr,fallback);
                        selected=true; break;
                    }
                }
                if (!selected) throw std::runtime_error("No non-C message locale available");
            }
        }
#endif
        // Explicit app selection must work even when the launcher sets LANG=C.
#ifdef _WIN32
        if (!std::setlocale(LC_MESSAGES, catalog.language.empty() ? "C" : catalog.language.c_str()))
            throw std::runtime_error("Unable to select gettext message locale");
#endif
        setEnvironment("LANGUAGE",
#ifdef _WIN32
            L"LANGUAGE",
#else
            nullptr,
#endif
            catalog.language);
        textdomain(catalog.domain.empty() ? "nijiexpose" : catalog.domain.c_str());
    }
};
Catalogs& catalogs() { static Catalogs state; return state; }
}
bool i18nValidateCultureCode(std::string_view code) {
    if (code == "C") return true;
    if (code.size() < 2) return false;
#ifdef _WIN32
    return !localeInfo(code, LOCALE_SISO639LANGNAME).empty();
#else
    return !localeInfo(code).empty();
#endif
}
std::string i18nGetCultureLanguage(std::string_view code) {
#ifdef _WIN32
    return localeInfo(code, LOCALE_SENGLISHLANGUAGENAME);
#else
    return localeInfo(code);
#endif
}
std::string i18nGetCultureNativeLanguageEstimate(std::string_view code) {
#ifdef _WIN32
    return localeInfo(code, LOCALE_SNATIVELANGUAGENAME);
#else
    return localeInfo(code);
#endif
}
std::string i18nGetCultureCountry(std::string_view code) {
#ifdef _WIN32
    return localeInfo(code, LOCALE_SENGLISHCOUNTRYNAME);
#else
    return localeInfo(code,true);
#endif
}
std::string i18nGetLanguageName(std::string_view file) {
    auto& state = catalogs();
    const auto temporary = state.stage(file);
    state.activate(temporary);
    std::string result(dgettext(temporary.domain.c_str(), "LANG_NAME"));
    state.activate(state.loaded);
    const auto code = nijiexpose::dcompat::pathToUTF8(path(file).stem());
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
    auto extension = nijiexpose::dcompat::pathToUTF8(path(file).extension());
    for (auto& c : extension) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if (extension != ".mo") throw std::runtime_error("i18n: Invalid file format " + nijiexpose::dcompat::pathToUTF8(path(file).extension()));
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
    return state.cStrings.emplace(_(text)).first->c_str();
}
}
