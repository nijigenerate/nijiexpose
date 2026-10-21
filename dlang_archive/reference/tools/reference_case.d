import std.uni : toLower, toUpper;
import std.utf : toUTF8;
import std.stdio;
import std.format;
void main(){
 writeln("// LDC 1.41 Phobos std.uni string case mapping (Boost license).\n#pragma once\n#include <cstdint>\nnamespace nijiexpose::dcompat {\nstruct CasePair {uint32_t from;const char* to;};");
 foreach(upper;[false,true]){
  writeln("inline constexpr CasePair ",upper?"upperStrings":"lowerStrings","[]{");
  foreach(uint cp;0..0x110000){if(cp>=0xd800&&cp<=0xdfff)continue;auto text=toUTF8([cast(dchar)cp]);auto mapped=upper?toUpper(text):toLower(text);if(text==mapped)continue;writef("{0x%x,\"",cp);foreach(ubyte b;cast(ubyte[])mapped)writef("\\x%02x",b);writeln("\"},");}
  writeln("};");
 }
 writeln("}");
}
