#include "platform/session.hpp"
#include "runtime/clock.hpp"

#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string_view>

namespace {
int run(bool smokeTest) {
    nijiexpose::Session session(smokeTest);
    nijiexpose::RuntimeClock runtime;
    bool running = true;
    int frames = 0;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                 event.window.windowID == SDL_GetWindowID(session.window()))) running = false;
        }
        if (!running) break;
        if (smokeTest && frames == 1 && !SDL_SetWindowSize(session.window(), 960, 540))
            throw std::runtime_error(SDL_GetError());

        const double delta = runtime.tick();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(24, 24), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(540, 320), ImGuiCond_FirstUseEver);
        ImGui::Begin("nijiexpose Native");
        ImGui::TextUnformatted("C++20 / nicxlive / SDL3 / OpenGL");
        ImGui::Separator();
        ImGui::TextWrapped("Development harness only. This is not a D-equivalent application port. Model rendering, tracking and streaming are not implemented.");
        ImGui::Text("nicxlive frame delta: %.3f ms", delta * 1000.0);
        ImGui::Text("SDL: %d.%d.%d", SDL_VERSIONNUM_MAJOR(SDL_GetVersion()),
            SDL_VERSIONNUM_MINOR(SDL_GetVersion()), SDL_VERSIONNUM_MICRO(SDL_GetVersion()));
        static char input[256] = "";
        ImGui::InputText("Input / IME", input, sizeof(input));
        if (ImGui::Button("Quit")) running = false;
        ImGui::End();
        ImGui::Render();

        int width = 0, height = 0;
        if (!SDL_GetWindowSizeInPixels(session.window(), &width, &height))
            throw std::runtime_error(SDL_GetError());
        if (width > 0 && height > 0) {
            glViewport(0, 0, width, height);
            glClearColor(0.055f, 0.065f, 0.09f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if (smokeTest) {
                // Read the untouched corner to verify the framebuffer, not just GL calls.
                unsigned char pixel[4] = {};
                glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
                if (glGetError() != GL_NO_ERROR)
                    throw std::runtime_error("OpenGL error during smoke test");
                if (std::abs(int(pixel[0]) - 14) > 2 || std::abs(int(pixel[1]) - 17) > 2 ||
                    std::abs(int(pixel[2]) - 23) > 2 || pixel[3] != 255)
                    throw std::runtime_error("OpenGL framebuffer readback failed");
            }
            if (!SDL_GL_SwapWindow(session.window())) throw std::runtime_error(SDL_GetError());
        } else if (smokeTest) {
            throw std::runtime_error("Smoke test has no drawable pixels");
        } else {
            SDL_Delay(16);
        }
        ++frames;
        if (smokeTest && frames == 4) running = false;
    }
    if (smokeTest) {
        if (frames != 4) throw std::runtime_error("Smoke test interrupted before completing four frames");
        int width = 0, height = 0;
        if (!SDL_GetWindowSize(session.window(), &width, &height) || width != 960 || height != 540)
            throw std::runtime_error("Smoke test window did not resize to 960 x 540");
    }
    return 0;
}
}

int main(int argc, char** argv) {
    bool smokeTest = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view option(argv[i]);
        if (option == "--version") {
            std::puts("nijiexpose-native " NIJIEXPOSE_NATIVE_VERSION " / nicxlive / SDL3 / ImGui " IMGUI_VERSION " (docking) / Boost.JSON / GNU gettext");
            return 0;
        }
        if (option == "--help") {
            std::puts("Usage: nijiexpose-native [--version | --help | --smoke-test]");
            return 0;
        }
        if (option == "--smoke-test") smokeTest = true;
        else {
            std::fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 2;
        }
    }
    try {
        const int result = run(smokeTest);
        if (smokeTest) std::puts("SDL3 / ImGui / nicxlive smoke test passed (4 frames, readback, resize, clean shutdown)");
        return result;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "nijiexpose-native: %s\n", error.what());
        return 1;
    }
}
