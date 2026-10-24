#pragma once
#include <SDL3/SDL.h>
namespace nijiexpose::render {
inline thread_local bool transparentWindow=false;
void configureWindowTransparency(SDL_Window* window,bool enabled);
void setWindowMousePassthrough(SDL_Window* window,bool enabled);
bool windowPointerPixel(SDL_Window* window,int width,int height,int& x,int& y);
}
