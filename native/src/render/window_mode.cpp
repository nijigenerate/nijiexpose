#include "window_mode.hpp"

namespace nijiexpose::render {
bool windowPointerPixel(SDL_Window* window, int width, int height, int& x, int& y) {
    float gx=0, gy=0;
    SDL_GetGlobalMouseState(&gx,&gy);
    int wx=0, wy=0, w=0, h=0;
    SDL_GetWindowPosition(window,&wx,&wy);
    SDL_GetWindowSize(window,&w,&h);
    if (w<=0 || h<=0 || gx<wx || gy<wy || gx>=wx+w || gy>=wy+h) return false;
    x=static_cast<int>((gx-wx)*width/w);
    y=static_cast<int>((gy-wy)*height/h);
    return true;
}
}
