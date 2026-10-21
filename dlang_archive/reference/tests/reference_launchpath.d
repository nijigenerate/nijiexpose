// Execute the original module, not a rewritten D approximation.
import nijiexpose.utils.launchpath;
import std.string : splitLines;
import std.stdio;
import std.conv : to;

void main() {
    foreach (line; stdin.byLine) {
        if (line.length < 2) continue;
        const operation = line[0];
        string input;
        for (size_t i = 2; i + 1 < line.length; i += 2)
            input ~= cast(char)to!ubyte(line[i..i+2], 16);
        try {
            if (operation == 'A') writeln(executableFileAvailable(input) ? "1" : "0");
            else if (operation == 'S') {
                foreach (value; splitLines(input)) {
                    foreach (ubyte c; cast(immutable(ubyte)[])value) writef("%02x", c);
                    write("|");
                }
                writeln();
            } else {
                foreach (ubyte c; cast(immutable(ubyte)[])launchPath(input)) writef("%02x", c);
                writeln();
            }
        } catch (Exception) { writeln("EXCEPTION"); }
    }
}
