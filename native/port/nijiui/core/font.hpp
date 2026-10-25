#pragma once
#include <string>
namespace nijiui {
void inInitFonts();
void inSetUIScale(float scale);
float inGetUIScaleFont();
float inGetUIScale();
std::string inGetUIScaleText();
void inFontsBeginLarge();
void inFontsEndLarge();
struct FontEntry { std::string name,lang,file; };
}
