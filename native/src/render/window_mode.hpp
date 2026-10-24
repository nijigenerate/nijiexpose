#pragma once
#include <SDL3/SDL.h>
namespace nijiexpose::render {
inline thread_local bool transparentWindow=false;
void configureWindowTransparency(SDL_Window* window,bool enabled);
void setWindowMousePassthrough(SDL_Window* window,bool enabled);
void processWindowPointerEvent(SDL_Window* window,const SDL_Event& event);
void updateWindowMousePassthrough(SDL_Window* window,bool transparent,bool sampleValid=true);
bool windowPointerPixel(SDL_Window* window,int width,int height,int& x,int& y);
}
