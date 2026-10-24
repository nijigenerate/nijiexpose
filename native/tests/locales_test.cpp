#include "i18n/tr.hpp"
#include "i18n.hpp"
#include "nijiui/core/app.hpp"
#include "nijiui/core/settings.hpp"
#include "dcompat/string.hpp"
#include <cstdio>
#include <stdexcept>
#define CHECK(condition) do { if (!(condition)) throw std::runtime_error(#condition); } while (0)
int wmain(int argc, wchar_t** argv) {
    try {
        if (argc != 2) return 2;
        using namespace i18n;
        const auto filename = nijiexpose::dcompat::toUTF8(argv[1]);
        CHECK(i18nLoadLanguage(filename));
        CHECK(_("hello") == "こんにちは");
        CHECK(std::string(__("hello")) == "こんにちは");
        const char* held = __("hello");
        for (int i=0;i<10000;++i) CHECK(__("hello")==held);
        for (int i=0;i<1000;++i) __("unique-"+std::to_string(i));
        CHECK(std::string(held)=="こんにちは");
        CHECK(_("missing") == "missing");
        CHECK(i18nGetLanguageName(filename) == "日本語");
        CHECK(_("hello") == "こんにちは");
        CHECK(!i18nLoadLanguage(filename + ".missing.mo"));
        CHECK(_("hello") == "こんにちは");
        i18nClearLanguage();
        CHECK(_("hello") == "hello");
        CHECK(std::string(held) == "こんにちは");
        CHECK(i18nLoadLanguage(filename));
        CHECK(_("hello") == "こんにちは");
        CHECK(i18nLoadLanguage(""));
        CHECK(_("hello") == "hello");

        using namespace nijiexpose;
        nijiui::inSetApplication({"net.nijilive.nijiexpose", "nijiexpose", "nijiexpose"});
        nijiui::inSettingsLoad();
        neLocaleInitFromSettings();
        CHECK(neLocaleCurrentCode() == "ja");
        CHECK(neLocaleCurrentName() == "日本語");
        CHECK(_("hello") == "こんにちは");
        CHECK(neLocaleGetEntryFor("ja") != nullptr);
        neLocaleSet("en");
        CHECK(_("hello") == "hello");
        neLocaleSet("ja");
        CHECK(_("hello") == "こんにちは");
        neLocaleSet("de");
        CHECK(_("hello") == "Hallo");
        neLocaleSet("ja");
        CHECK(_("hello") == "こんにちは");
        neLocaleSet("zz"); // Source stores unknown code but retains the loaded language.
        CHECK(neLocaleCurrentCode() == "zz");
        CHECK(_("hello") == "こんにちは");
        neLocaleSet("");
        CHECK(_("hello") == "hello");
        nijiui::inSettingsSave();
        std::vector<TLEntry> entries{{"same", "ja", "", "a"}, {"same", "de", "", "b"}, {"same", "en", "", "c"}};
        neMarkLocaleDups(entries);
        CHECK(entries[0].humanName == "same (ja)" && entries[1].humanName == "same (de)" && entries[2].humanName == "same (en)");
        std::puts("GNU gettext and locale settings integration passed");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
