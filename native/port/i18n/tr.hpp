#pragma once
#include <string>
#include <string_view>
namespace i18n {
std::string i18nGetLanguageName(std::string_view file);
void i18nClearLanguage();
bool i18nLoadLanguage(std::string_view file);
std::string _(std::string_view text);
const char* __(std::string_view text);
std::string i18nGetCultureLanguage(std::string_view code);
std::string i18nGetCultureNativeLanguageEstimate(std::string_view code);
std::string i18nGetCultureCountry(std::string_view code);
bool i18nValidateCultureCode(std::string_view code);
}
