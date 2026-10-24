// Windows path of nijiui/core/package.d, translated to SDL3.
#include "core.hpp"
#include "settings.hpp"
#include <SDL3/SDL.h>
#include <cmath>
namespace nijiui {
namespace { thread_local double lastTime,currTime; }
void inInitUI() {
    // SDL3 subsumes timer/events initialization and removed the SDL2 EVERYTHING constant.
    SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_HAPTIC|SDL_INIT_GAMEPAD|SDL_INIT_EVENTS|SDL_INIT_SENSOR);
    inSettingsLoad();
}
void inUpdateTime() { lastTime=currTime; currTime=static_cast<double>(SDL_GetTicks())*.001; }
double inGetTime() { return currTime; }
double inGetDeltaTime() { return std::abs(lastTime-currTime); }
}
