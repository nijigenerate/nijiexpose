module nijiexpose.i18n;

import i18n;
import i18n.culture;
import i18n.tr;
import nijiui.core.path;
import nijiui.core.settings;
import std.algorithm : sort;
import std.file;
import std.path;
import std.string;
import std.uni : icmp;

private {
    TLEntry[] localeFiles;

    string neLocaleLanguageNameMarker() {
        return _("LANG_NAME");
    }

    string neGetCultureExpression(string langcode) {
        foreach (locale; localeFiles) {
            if (locale.code == langcode) {
                return locale.humanName;
            }
        }

        if (langcode.length >= 5) {
            return format("%s (%s)", i18nGetCultureLanguage(langcode),
                langcode == "zh-CN" ? "Simplified" :
                langcode == "zh-TW" ? "Traditional" :
                i18nGetCultureCountry(langcode));
        }
        return i18nGetCultureLanguage(langcode);
    }

    void neLocaleScan(string path) {
        if (!path.exists) return;

        foreach (DirEntry entry; dirEntries(path, "*.mo", SpanMode.shallow)) {
            string langcode = baseName(stripExtension(entry.name));
            if (!i18nValidateCultureCode(langcode)) continue;

            string langName = i18nGetLanguageName(entry.name);
            if (langName == "<UNKNOWN LANGUAGE>") langName = neGetCultureExpression(langcode);

            bool found;
            foreach (locale; localeFiles) {
                if (locale.code == langcode) {
                    found = true;
                    break;
                }
            }
            if (found) continue;

            localeFiles ~= TLEntry(
                langName,
                langName.toStringz,
                langcode,
                entry.name,
                path
            );
        }
    }
}

struct TLEntry {
    string humanName;
    const(char)* humanNameC;
    string code;
    string file;
    string path;
}

void neLocaleInit() {
    localeFiles = null;

    neLocaleScan(inGetAppLocalePath());
    neLocaleScan(thisExePath().dirName);

    version(Windows) neLocaleScan(buildPath(thisExePath().dirName, "i18n"));
    version(OSX) neLocaleScan(buildPath(thisExePath().dirName, "../Resources/i18n"));

    localeFiles.sort!(neCompareLocaleEntries);
    neMarkLocaleDups(localeFiles);
}

void neLocaleInitFromSettings() {
    neLocaleInit();
    if (inSettingsCanGet("lang")) {
        string lang = inSettingsGet!string("lang");
        auto entry = neLocaleGetEntryFor(lang);
        if (entry !is null) {
            i18nLoadLanguage(entry.file);
        }
    }
}

bool neCompareLocaleEntries(TLEntry a, TLEntry b) {
    int cmp = icmp(a.humanName, b.humanName);
    if (cmp == 0) {
        return a.path < b.path;
    }
    return cmp < 0;
}

void neMarkLocaleDups(TLEntry[] entries) {
    if (entries.length <= 1) return;

    TLEntry* prevEntry = &entries[0];
    bool prevIsDup = false;

    foreach (ref entry; entries[1 .. $]) {
        bool entryIsDup = entry.humanName == prevEntry.humanName;

        if (prevIsDup || entryIsDup) {
            prevEntry.humanName ~= " (" ~ prevEntry.code ~ ")";
            prevEntry.humanNameC = prevEntry.humanName.toStringz;
        }
        prevIsDup = entryIsDup;
        prevEntry = &entry;
    }

    if (prevIsDup) {
        prevEntry.humanName ~= " (" ~ prevEntry.code ~ ")";
        prevEntry.humanNameC = prevEntry.humanName.toStringz;
    }
}

string neLocaleCurrentCode() {
    return inSettingsGet("lang", "en");
}

string neLocaleCurrentName() {
    string code = neLocaleCurrentCode();
    string currCode = code.length == 0 ? "en" : code;
    return neGetCultureExpression(currCode);
}

void neLocaleSet(string code) {
    inSettingsSet("lang", code);

    if (code.length == 0 || code == "en") {
        i18nClearLanguage();
        return;
    }

    auto entry = neLocaleGetEntryFor(code);
    if (entry !is null) {
        i18nLoadLanguage(entry.file);
    }
}

TLEntry* neLocaleGetEntryFor(string code) {
    foreach (ref entry; localeFiles) {
        if (entry.code == code) return &entry;
    }
    return null;
}

TLEntry[] neLocaleGetEntries() {
    return localeFiles;
}
