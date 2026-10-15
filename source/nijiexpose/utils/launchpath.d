module nijiexpose.utils.launchpath;

import std.string : fromStringz, strip;

// Windows' Copy as path includes quotes. Keep the editable text intact, but
// remove its wrapping quotes before filesystem checks or process construction.
string launchPath(string input) {
    auto path = input.fromStringz.strip;
    if (path.length >= 2 && path[0] == '"' && path[$ - 1] == '"') {
        path = path[1 .. $ - 1];
    }
    return path;
}

unittest {
    assert(launchPath(null) == "");
    assert(launchPath(`  "C:\Tracker Files\nijitrack.exe"  `) == `C:\Tracker Files\nijitrack.exe`);
    assert(launchPath("\"C:\\日本語\\python.exe\"\0padding") == "C:\\日本語\\python.exe");
    assert(launchPath(`C:\Tracker Files\nijitrack.exe`) == `C:\Tracker Files\nijitrack.exe`);
}
