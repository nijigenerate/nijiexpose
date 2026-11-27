#include "nijilive/opengl/backend.hpp"
#include "nijimi/opengl/present.hpp"
#include <core/runtime_state.hpp>
#include <SDL3/SDL.h>
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace gl = nijiexpose::ogl;
namespace core = nicxlive::core;
namespace {
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
}

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << SDL_GetError() << '\n';
        return 1;
    }
    int result = 0;
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
    try {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        constexpr int size = 64;
        window = SDL_CreateWindow("Presentation alpha test", size, size,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_TRANSPARENT);
        require(window != nullptr, SDL_GetError());
        context = SDL_GL_CreateContext(window);
        require(context != nullptr, SDL_GetError());
        auto backend = std::make_shared<gl::Backend>();
        core::inSetRenderBackend(backend);
        backend->initializeRenderer();
        core::inSetViewport(size, size);

        core::inSetClearColor(.25f, .125f, 0, .5f);
        backend->beginScene();
        auto readPresented = [&](gl::PresentationMode mode) {
            gl::oglPresentSceneToBackbuffer(size, size, mode);
            gl::glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            gl::glReadBuffer(GL_BACK);
            std::array<unsigned char, 4> pixel{};
            glReadPixels(size / 2, size / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
            return pixel;
        };
        const auto opaque = readPresented(gl::PresentationMode::Opaque);
        const auto transparent = readPresented(gl::PresentationMode::Transparent);
        require(opaque[3] == 255, "Normal window presented a translucent pixel");
        require(std::abs(int(transparent[3]) - 128) <= 1,
                "Presentation changed the alpha of the transparent scene");
        for (int channel = 0; channel < 3; ++channel)
            require(opaque[channel] == transparent[channel], "Opaque mode changed scene colors");
        const auto keyed = readPresented(gl::PresentationMode::ColorKey);
        require(keyed[3] == 255 && std::abs(int(keyed[0]) - 128) <= 1,
                "Color-key mode lost the straight-alpha model color");
        core::inSetClearColor(0, 0, 0, 0);
        backend->beginScene();
        require(readPresented(gl::PresentationMode::ColorKey) == std::array<unsigned char, 4>{255, 0, 255, 255},
                "Color-key mode did not replace the transparent background");
        std::cout << "Opaque, transparent and color-key presentation alpha passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    core::inSetRenderBackend(nullptr);
    if (context) SDL_GL_DestroyContext(context);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
