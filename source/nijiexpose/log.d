/*
    Copyright © 2022, Inochi2D Project
    Copyright © 2024, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
*/
module nijiexpose.log;
import std.stdio : stdout, stderr;
import std.format : format;

version (Windows) {
    import core.stdc.stdio : FILE;
    private extern(C) FILE* _wfreopen(const(wchar)*, const(wchar)*, FILE*);
}

// Initialize before UI, tracking, or plugins can write to standard streams.
// Reopen the CRT streams too, so dependency writes work in GUI builds.
void insInitLogging() {
    version (Windows) {
        import std.process : environment;
        import std.path : buildPath;
        import std.file : mkdirRecurse, exists, getSize, remove, rename;
        import std.utf : toUTF16z;
        import core.stdc.stdio : FILE, setvbuf, _IONBF, fileno, _get_osfhandle;
        import core.sys.windows.windows : SetStdHandle, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE, HANDLE;

        string destination = "NUL";
        try {
            auto root = environment.get("LOCALAPPDATA", "");
            if (root.length) {
                auto directory = buildPath(root, "nijiexpose", "logs");
                mkdirRecurse(directory);
                auto path = buildPath(directory, "nijiexpose.log");
                // Bound accumulated logs; retain the previous log for diagnostics.
                if (exists(path) && getSize(path) >= 5 * 1024 * 1024) {
                    auto previous = path ~ ".previous";
                    if (exists(previous)) remove(previous);
                    rename(path, previous);
                }
                destination = path;
            }
        } catch (Exception) {
            // Logging must never prevent the application from starting.
        }
        foreach (index, stream; [stdout.getFP(), stderr.getFP()]) {
            auto reopened = _wfreopen(destination.toUTF16z, "a".toUTF16z, stream);
            if (reopened is null) {
                // Program Files and unavailable user directories must not leave
                // a console-less process writing to an invalid CRT stream.
                reopened = _wfreopen("NUL".toUTF16z, "a".toUTF16z, stream);
            }
            if (reopened !is null) {
                setvbuf(reopened, null, _IONBF, 0);
                SetStdHandle(index == 0 ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE,
                    cast(HANDLE)_get_osfhandle(fileno(reopened)));
            }
        }
    }
}

private void insLog(T...)(string level, string fmt, T args) {
    try {
        version (Windows) {
            stdout.writeln("[", level, "] ", format(fmt, args));
            stdout.flush();
        } else {
            version (nijiexposeConsoleLog) {
                stdout.writeln("[", level, "] ", format(fmt, args));
                stdout.flush();
            }
        }
    } catch (Exception) {
        // Disk full, a closed stream, or a malformed message is not fatal.
    }
}

void insLogDebug(T...)(string fmt, T args) { insLog("DEBUG", fmt, args); }
void insLogInfo(T...)(string fmt, T args) { insLog("INFO", fmt, args); }
void insLogWarn(T...)(string fmt, T args) { insLog("WARN", fmt, args); }
void insLogErr(T...)(string fmt, T args) { insLog("ERR ", fmt, args); }
