import nijiexpose.utils.subprocess;
import std.stdio;

void main(string[] args) {
    if (args[1] == "capture") {
        auto process = new SubProcess!true(args[2], ["arg with space", "quote\"tail", "end\\", "日本語", "\0tail"]);
        writeln("code:", process.execute());
        foreach (line; process.stdout) {
            foreach (ubyte c; cast(immutable(ubyte)[])line) writef("%02x", c);
            writeln("|");
        }
    } else version (CheckOriginalRawDataCompilation) {
        auto process = new SubProcess!(true, true)(args[2], ["arg with space"]);
        writeln("code:", process.execute());
        foreach (ubyte c; process.stdout) writef("%02x", c);
        writeln();
    } else if (args[1] == "missing") {
        auto process = new SubProcess!false("Z:\\nijiexpose-missing-fixture.exe");
        writeln("code:", process.execute());
    } else if (args[1] == "terminate") {
        auto process = new SubProcess!false(args[2], ["sleep"]);
        writeln("start:", process.start());
        process.terminate();
        writeln("running:", process.running(), ",code:", process.getExitCode());
    } else if (args[1] == "rerun") {
        auto process = new SubProcess!false(args[2]);
        writeln("code:", process.execute());
        process.rerun();
        while (process.running()) process.update();
        writeln("code:", process.getExitCode());
    } else {
        auto process = new SubProcess!false(args[2]);
        writeln("code:", process.execute());
    }
}
