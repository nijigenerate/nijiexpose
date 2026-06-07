/*
    Copyright © 2022, Inochi2D Project
    Copyright © 2024, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.log;
import std.stdio : writeln;
import std.format;

version(nijiexposeConsoleLog) {
    private enum bool logToConsole = true;
} else {
    private enum bool logToConsole = false;
}

void insLogDebug(T...)(string fmt, T args) {
    static if (logToConsole) {
        writeln("[DEBUG] ", fmt.format(args));
    }
}

void insLogInfo(T...)(string fmt, T args) {
    static if (logToConsole) {
        writeln("[INFO] ", fmt.format(args));
    }
}

void insLogWarn(T...)(string fmt, T args) {
    static if (logToConsole) {
        writeln("[WARN] ", fmt.format(args));
    }
}

void insLogErr(T...)(string fmt, T args) {
    static if (logToConsole) {
        writeln("[ERR ] ", fmt.format(args));
    }
}
