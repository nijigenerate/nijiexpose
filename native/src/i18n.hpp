#pragma once
#include <span>
#include <string>
#include <string_view>
namespace nijiexpose {
struct TLEntry {
    std::string humanName, code, file, path;
    const char* humanNameC() const { return humanName.c_str(); }
};
void neLocaleInit();
void neLocaleInitFromSettings();
bool neCompareLocaleEntries(const TLEntry& a, const TLEntry& b);
void neMarkLocaleDups(std::span<TLEntry> entries);
std::string neLocaleCurrentCode();
std::string neLocaleCurrentName();
void neLocaleSet(std::string code);
TLEntry* neLocaleGetEntryFor(std::string_view code);
std::span<TLEntry> neLocaleGetEntries();
}
