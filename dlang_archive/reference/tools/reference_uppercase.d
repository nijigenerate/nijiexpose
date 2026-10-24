module reference_uppercase;
import std.uni : toUpper;
import std.stdio;
void main() {
    writeln("// Unicode simple uppercase mapping from LDC 1.41 Phobos std.uni (Boost license).");
    writeln("#pragma once\n#include <cstdint>\nnamespace nijiexpose::dcompat {\nstruct UppercasePair { uint32_t from,to; };\ninline constexpr UppercasePair uppercaseMap[]{");
    foreach(uint code;0xAA..0x110000) {
        auto upper=toUpper(cast(dchar)code);
        if(code!=upper) writefln("{0x%x,0x%x},",code,cast(uint)upper);
    }
    writeln("};\n}");
}
