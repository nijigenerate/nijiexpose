import nijiexpose.log;
import std.stdio;
void main() {
    insInitLogging();
    insLogDebug("debug");
    insLogInfo("hello %s", "日本語");
    insLogWarn("two %s %s", "strings", "test");
    insLogErr("error %s", 42);
    insLogInfo("args=%s", ["a", "line\nnext", "tab\tx", "quote\"", "nul\0tail", "\\"]);
    string controls;
    foreach (i; 0..32) controls ~= cast(char)i;
    insLogInfo("controls=%s", [controls]);
    insLogInfo("broken %s");
    insLogInfo("percent %%");
    stdout.writeln("stdout dependency");
    stderr.writeln("stderr dependency");
}
