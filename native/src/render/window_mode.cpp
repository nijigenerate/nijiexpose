#include "window_mode.hpp"

namespace nijiexpose::render {
namespace {
constexpr auto buttonsKey="nijiexpose.window.pointer_buttons";
constexpr auto transparentPixelKey="nijiexpose.window.transparent_pixel";
}
void processWindowPointerEvent(SDL_Window* window,const SDL_Event& event) {
    const auto props=SDL_GetWindowProperties(window);
    const auto id=SDL_GetWindowID(window);
    if (event.type==SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID==id) {
        SDL_ClearProperty(props,buttonsKey);
    } else if ((event.type==SDL_EVENT_MOUSE_BUTTON_DOWN || event.type==SDL_EVENT_MOUSE_BUTTON_UP)
               && event.button.windowID==id) {
        auto buttons=static_cast<SDL_MouseButtonFlags>(SDL_GetNumberProperty(props,buttonsKey,0));
        const auto button=SDL_BUTTON_MASK(event.button.button);
        if (event.type==SDL_EVENT_MOUSE_BUTTON_DOWN) buttons|=button;
        else buttons&=~button;
        SDL_SetNumberProperty(props,buttonsKey,buttons);
    }
}
void updateWindowMousePassthrough(SDL_Window* window,bool transparent,bool sampleValid) {
    const auto props=SDL_GetWindowProperties(window);
    const auto pressed=SDL_GetGlobalMouseState(nullptr,nullptr);
    // Only button presses delivered to this window own a drag. Global state
    // also includes drags begun in other applications or on the desktop.
    const auto owned=static_cast<SDL_MouseButtonFlags>(SDL_GetNumberProperty(props,buttonsKey,0))&pressed;
    SDL_SetNumberProperty(props,buttonsKey,owned);
    if (!transparentWindow) {
        SDL_ClearProperty(props,transparentPixelKey);
        setWindowMousePassthrough(window,false);
        return;
    }
    if (sampleValid) SDL_SetBooleanProperty(props,transparentPixelKey,transparent);
    // Keep the last hit result while Vulkan's next pixel sample is pending.
    // A moving pointer must not temporarily make the whole overlay intercept input.
    const bool through=SDL_GetBooleanProperty(props,transparentPixelKey,true);
    setWindowMousePassthrough(window,!owned && (pressed || through));
}
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
