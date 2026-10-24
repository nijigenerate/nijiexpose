#include "platform/session.hpp"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <stdexcept>
#include <string>

namespace nijiexpose {
namespace {
void require(bool success, const char* operation) {
    if (!success) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}
}

Session::Session(bool smokeTest) {
    try {
        require(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");
        sdl_ = true;
        require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE), "GL profile");
#ifdef __APPLE__
        require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG), "GL flags");
        require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4), "GL major version");
        require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1), "GL minor version");
#else
        require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3), "GL major version");
        require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3), "GL minor version");
#endif
        require(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1), "GL doublebuffer");
        require(SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8), "GL stencil size");
        auto flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        if (smokeTest) flags |= SDL_WINDOW_HIDDEN;
        window_ = SDL_CreateWindow("nijiexpose Native - C++ / SDL3", 1280, 720, flags);
        require(window_ != nullptr, "SDL_CreateWindow");
        context_ = SDL_GL_CreateContext(window_);
        require(context_ != nullptr, "SDL_GL_CreateContext");
        require(SDL_GL_MakeCurrent(window_, context_), "SDL_GL_MakeCurrent");
        if (!SDL_GL_SetSwapInterval(smokeTest ? 0 : 1)) {
            SDL_Log("VSync unavailable: %s", SDL_GetError());
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        imgui_ = true;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Smoke tests must not depend on or modify the user's layout.
        if (smokeTest) ImGui::GetIO().IniFilename = nullptr;
        ImGui::StyleColorsDark();
        if (!ImGui_ImplSDL3_InitForOpenGL(window_, context_))
            throw std::runtime_error("ImGui SDL3 initialization failed");
        platform_ = true;
        if (!ImGui_ImplOpenGL3_Init("#version 330"))
            throw std::runtime_error("ImGui OpenGL3 initialization failed");
        renderer_ = true;
    } catch (...) {
        close();
        throw;
    }
}

Session::~Session() { close(); }

void Session::close() noexcept {
    if (renderer_) ImGui_ImplOpenGL3_Shutdown();
    if (platform_) ImGui_ImplSDL3_Shutdown();
    if (imgui_) ImGui::DestroyContext();
    if (context_) SDL_GL_DestroyContext(context_);
    if (window_) SDL_DestroyWindow(window_);
    if (sdl_) SDL_Quit();
    renderer_ = platform_ = imgui_ = sdl_ = false;
    context_ = nullptr;
    window_ = nullptr;
}

} // namespace nijiexpose
