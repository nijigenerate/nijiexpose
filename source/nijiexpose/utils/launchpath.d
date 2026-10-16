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

bool executableFileAvailable(string input) {
    import std.file : exists, isFile;
    auto path = launchPath(input);
    try {
        if (!path.length || !exists(path) || !isFile(path)) return false;
        version (Posix) {
            import core.sys.posix.unistd : access, X_OK;
            import std.string : toStringz;
            return access(path.toStringz, X_OK) == 0;
        } else {
            return true;
        }
    } catch (Exception) {
        return false;
    }
}

unittest {
    import std.file : thisExePath;
    import std.path : buildPath, dirName;
    auto root = buildPath(dirName(thisExePath()), "nijitrack");
    auto nativePath = buildPath(root, "Tracker Files", "nijitrack.exe");
    auto pythonPath = buildPath(root, "日本語", "python.exe");
    assert(launchPath(null) == "");
    assert(launchPath("  \"" ~ nativePath ~ "\"  ") == nativePath);
    assert(launchPath("\"" ~ pythonPath ~ "\"\0padding") == pythonPath);
    assert(launchPath(nativePath) == nativePath);
}
