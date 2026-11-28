#pragma once
#include <SDL3/SDL.h>
namespace nijiexpose::render {
inline thread_local bool transparentWindow=false;
#if defined(__linux__) && defined(NIJIEXPOSE_VULKAN)
SDL_Window* createLinuxVulkanWindow(const char* title,int width,int height,SDL_WindowFlags flags);
#endif
void configureWindowTransparency(SDL_Window* window,bool enabled);
#ifdef _WIN32
void restoreWindowDropEvents(SDL_Window* window);
#endif
void setWindowMousePassthrough(SDL_Window* window,bool enabled);
void processWindowPointerEvent(SDL_Window* window,const SDL_Event& event);
void updateWindowMousePassthrough(SDL_Window* window,bool transparent,bool sampleValid=true);
bool windowPointerPixel(SDL_Window* window,int width,int height,int& x,int& y);
}
