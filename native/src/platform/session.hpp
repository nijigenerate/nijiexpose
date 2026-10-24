#pragma once

#include <SDL3/SDL.h>

namespace nijiexpose {

class Session {
public:
    explicit Session(bool smokeTest);
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    SDL_Window* window() const { return window_; }

private:
    void close() noexcept;
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    bool sdl_ = false;
    bool imgui_ = false;
    bool platform_ = false;
    bool renderer_ = false;
};

} // namespace nijiexpose
