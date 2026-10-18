import nijiui.core.app;
import nijiui.core.settings;
import std.stdio;
void emit(string value) { foreach (ubyte ch; cast(ubyte[])value) writef("%02x", ch); }
void main() {
    inSetApplication(InApplication("net.nijilive.nijiexpose", "nijiexpose", "nijiexpose"));
    try {
        inSettingsLoad();
        auto scale = inSettingsGet!float("UIScale", 1.0f);
        auto language = inSettingsGet!string("lang");
        auto color = inSettingsGet!(float[4])("bgColor", [0.5f, 0.5f, 0.5f, 0]);
        writef("%s %s %08x %s ", cast(int)inSettingsCanGet("UIScale"), cast(int)inSettingsGet!bool("visible", true),
            *cast(uint*)&scale, cast(int)(language !is null));
        emit(language); write(" ");
        foreach (item; color) writef("%08x ", *cast(uint*)&item);
        writeln;
        inSettingsSet("new", 42);
        inSettingsSet("lang", "ja");
        inSettingsSave();
        inSettingsLoad();
        writef("%s ", inSettingsGet!int("new")); emit(inSettingsGet!string("lang")); writeln;
    } catch (Exception) { writeln("Exception"); }
}
