"""Translate the installed original bindbc-spout2 C ABI declarations."""
from pathlib import Path
import re
source=Path('C:/Users/siget/AppData/Local/dub/packages/bindbc-spout2/0.1.1/bindbc-spout2/source/bindbc/spout2/package.d').read_text(encoding='utf-8')
header='''// ABI port of bindbc-spout2 0.1.1. See native/licenses/bindbc-spout2.txt.
#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
namespace nijiexpose::spout {
struct SPOUTLIBRARY;
using SPOUTHANDLE=SPOUTLIBRARY*;
enum class Spout2Support { noLibrary,badLibrary,spout2 };
Spout2Support loadSpout2(const char* libName="SpoutLibrary.dll");
void unloadSpout2();
Spout2Support loadedSpout2Version();
bool isSpout2Loaded();
'''
impl='#include "spout.hpp"\nnamespace nijiexpose::spout {\n'
for name,ret,args in re.findall(r'alias _(\w+) = (.*?) function\((.*?)\);',source):
    def types(text):
        text=text.replace('const(char)*','const char*').replace('const(ubyte)*','const uint8_t*')
        text=re.sub(r'\buint\b','uint32_t',text)
        text=re.sub(r'\blong\b','int64_t',text)
        return re.sub(r'\bref (\w+) ',r'\1& ',text)
    header+=f'using {name}Type = {types(ret)} (__cdecl*)({types(args)});\nextern {name}Type {name};\n'
    impl+=f'{name}Type {name}=nullptr;\n'
header+='}\n'
impl+='''namespace { thread_local HMODULE lib=nullptr; thread_local Spout2Support loadedVersion=Spout2Support::noLibrary; }
void unloadSpout2() { if(lib) { FreeLibrary(lib); lib=nullptr; } }
Spout2Support loadedSpout2Version() { return loadedVersion; }
bool isSpout2Loaded() { return lib!=nullptr; }
Spout2Support loadSpout2(const char* libName) {
    lib=LoadLibraryA(libName);
    if(!lib) return Spout2Support::noLibrary;
    loadedVersion=Spout2Support::badLibrary;
'''
for var,symbol in re.findall(r'lib.bindSymbol\(cast\(void\*\*\)&(\w+), "(.*?)"\);',source):
    impl+=f'    {var}=reinterpret_cast<{var}Type>(GetProcAddress(lib,"{symbol}"));\n'
impl+='    return Spout2Support::spout2;\n}\n}\n'
Path('native/src/spout.hpp').write_text(header,encoding='utf-8')
Path('native/src/spout.cpp').write_text(impl,encoding='utf-8')
