import nijiui.core.app;
import nijiui.core.path;
import std.process : environment;
import std.stdio;
void main() {
    inSetApplication(InApplication("net.nijilive.nijiexpose", "nijiexpose", "nijiexpose"));
    try {
        writeln(inGetAppConfigPath());
        writeln(inGetAppImguiConfigFile());
        writeln(inGetAppFontsPath());
        writeln(inGetAppLocalePath());
        writeln(inGetAppCustomPath("plugins"));
        environment["IN_CONFIG_PATH"] = "changed-after-cache";
        writeln(inGetAppConfigPath());
    } catch (Exception) { writeln("ERROR"); }
}
