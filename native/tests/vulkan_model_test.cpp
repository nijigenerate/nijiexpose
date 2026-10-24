#include "nijilive/model_io.hpp"
#include "render/vulkan/backend.hpp"
#include <algorithm>
#include <core/runtime_state.hpp>
#include <core/timing.hpp>
#include <fstream>
#include <imgui_impl_sdl3.h>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include "framesend.hpp"
#include "spout_receiver.hpp"
#include <future>
#endif
namespace core = nicxlive::core;
namespace vk = nijiexpose::vulkan;
static void require(bool v, const char *message) {
  if (!v)
    throw std::runtime_error(message);
}
static double clockNow() { return double(SDL_GetTicks()) / 1000.; }
static void writePPM(const std::string &path, const std::vector<uint8_t> &p,
                     int w, int h) {
  std::ofstream f(path, std::ios::binary);
  f << "P6\n" << w << ' ' << h << "\n255\n";
  for (size_t i = 0; i < p.size(); i += 4)
    f.write(reinterpret_cast<const char *>(p.data() + i), 3);
}
int main(int argc, char **argv) {
  try {
    require(argc == 4,
            "usage: vulkan_model_test first.inp second.inp output-prefix");
    require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    auto *window = SDL_CreateWindow("Vulkan model integration test", 800, 800,
                                    SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN |
                                        SDL_WINDOW_RESIZABLE);
    require(window, SDL_GetError());
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    {
      vk::Context context(window);
      ImGui_ImplSDL3_InitForVulkan(window);
      context.initializeImGui();
      core::inInit(clockNow);
      auto backend = std::make_shared<vk::Backend>();
      core::inSetRenderBackend(backend);
      backend->initializeRenderer();
      core::inSetViewport(800, 800);
      core::inSetClearColor(.2, .2, .2, 0);
#ifdef _WIN32
      struct SenderScope {
        ~SenderScope() { nijiexpose::insCleanupFrameSending(); }
      } senderScope;
      nijiexpose::insInitFrameSending();
      require(nijiexpose::insCanSendFrames(),
              "Vulkan Spout sender initialization failed");
      VkPhysicalDeviceIDProperties id{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
      VkPhysicalDeviceProperties2 props{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
      props.pNext = &id;
      vkGetPhysicalDeviceProperties2(context.physical, &props);
      require(id.deviceLUIDValid, "Receiver requires device LUID");
#endif
      auto frame = [&](const std::shared_ptr<core::Puppet> &puppet) {
        SDL_PumpEvents();
        require(context.beginFrame(), "Cannot acquire frame");
        backend->resizeViewportTargets(context.width(), context.height());
        core::inSetViewport(context.width(), context.height());
#ifdef _WIN32
        if (backend->sceneImage().externalOwned)
          require(!backend->readTexture(backend->renderImageHandle()).empty(),
                  "Viewport read during acquired frame failed");
#endif
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(300, 100));
        ImGui::Begin("Verification");
        ImGui::TextUnformatted("Vulkan model + ImGui");
        ImGui::End();
        ImGui::Render();
        require(ImGui::GetDrawData()->TotalVtxCount > 0,
                "ImGui geometry absent");
        core::inUpdate();
        backend->beginScene();
        if (puppet) {
          puppet->update();
          backend->drawPuppet(*puppet);
        }
        backend->endScene();
        backend->render();
        context.present(ImGui::GetDrawData(), backend->sceneImage());
#ifdef _WIN32
        nijiexpose::insSendFrame();
        require(nijiexpose::insCanSendFrames(), "Vulkan Spout send failed");
#endif
        context.idle();
        auto pixels = backend->readTexture(backend->renderImageHandle());
#ifdef _WIN32
        auto received = spout_test::receive(
            nijiexpose::insFrameSenderName(), id.deviceLUID, context.width(),
            context.height(), backend->sceneImage().sharedHandle);
        if (pixels != received) {
          writePPM(std::string(argv[3]) + "-source.ppm", pixels,
                   context.width(), context.height());
          writePPM(std::string(argv[3]) + "-received.ppm", received,
                   context.width(), context.height());
          size_t flipped = 0;
          for (int y = 0; y < context.height(); ++y)
            for (int x = 0; x < context.width() * 4; ++x)
              flipped += pixels[size_t(y) * context.width() * 4 + x] !=
                         received[size_t(context.height() - 1 - y) *
                                      context.width() * 4 +
                                  x];
          std::cerr << "vertical-flip mismatch bytes: " << flipped << '\n';
          size_t changed = 0;
          for (size_t i = 0; i < pixels.size(); ++i)
            changed += pixels[i] != received[i];
          throw std::runtime_error("Spout receiver pixels differ: " +
                                   std::to_string(changed) + " bytes");
        }
#endif
        return pixels;
      };
      for (int model = 1; model <= 2; ++model) {
        auto puppet = nijiexpose::model_io::inLoadPuppet(argv[model]);
        puppet->root->build();
        core::nodes::inSetUpdateBounds(true);
        puppet->update();
        auto bounds = puppet->root->getCombinedBounds(true, true);
        auto &camera = core::inGetCamera();
        float scale =
            700 / std::max(bounds[2] - bounds[0], bounds[3] - bounds[1]);
        camera.scale = {scale, scale};
        camera.position = {(bounds[0] + bounds[2]) * .5f * scale,
                           (bounds[1] + bounds[3]) * .5f * scale};
        auto baseline = frame(puppet);
#ifdef _WIN32
        // A concurrent receiver must prevent us from rendering into its image.
        std::promise<bool> acquired;
        std::promise<void> resume;
        auto ready = acquired.get_future();
        auto proceed = resume.get_future();
        std::string mutexName = std::string(nijiexpose::insFrameSenderName()) + "_SpoutAccessMutex";
        auto receiver = std::async(std::launch::async, [&] {
          spout_test::Handle access;
          access.value = OpenMutexA(SYNCHRONIZE | MUTEX_MODIFY_STATE,FALSE,mutexName.c_str());
          access.locked = access.value && WaitForSingleObject(access.value,5000)==WAIT_OBJECT_0;
          acquired.set_value(access.locked);
          proceed.wait();
        });
        bool receiverLocked = ready.get();
        bool skipped = receiverLocked && !context.beginFrame();
        resume.set_value();
        receiver.get();
        require(receiverLocked && skipped,"Rendering did not respect receiver ownership");
        // Puppet physics continues advancing: compare the resumed frame with
        // its receiver in frame(), rather than requiring a frozen old pose.
        auto resumed = frame(puppet);
        size_t visibleAfterRelease = 0;
        for (size_t i=3;i<resumed.size();i+=4) visibleAfterRelease += resumed[i]>0;
        require(visibleAfterRelease>1000,"Rendering did not recover after receiver release");
#endif
        size_t opaque = 0;
        for (size_t i = 3; i < baseline.size(); i += 4)
          opaque += baseline[i] > 0;
        require(opaque > 1000, "Model rendered no visible geometry");
        writePPM(std::string(argv[3]) + "-" + std::to_string(model) + ".ppm",
                 baseline, context.width(), context.height());
        for (int i = 0; i < 24; ++i)
          frame(puppet);
        puppet->transform.translation.x += 50;
        puppet->transform.update();
        auto moved = frame(puppet);
        require(baseline != moved, "Model movement did not change pixels");
        SDL_SetWindowSize(window, 640, 720);
        context.resize();
        frame(puppet);
        require(context.width() == 640 && context.height() == 720,
                "Resize failed");
        std::weak_ptr<core::Puppet> released = puppet;
        puppet.reset();
        require(released.expired(), "Old puppet remains owned");
        auto empty = frame(nullptr);
        for (size_t i = 3; i < empty.size(); i += 4)
          require(empty[i] == 0, "Old model remains after deletion");
        SDL_SetWindowSize(window, 800, 800);
        context.resize();
        frame(nullptr);
        std::cout << "model " << model << ": visible pixels=" << opaque
                  << ", move, resize, delete passed\n";
      }
#ifdef _WIN32
      nijiexpose::insCleanupFrameSending();
      require(!nijiexpose::insCanSendFrames(),
              "Sender remains enabled after cleanup");
#endif
      backend->shutdown();
      core::inSetRenderBackend(nullptr);
      backend.reset();
      context.shutdownImGui();
      ImGui_ImplSDL3_Shutdown();
    }
    ImGui::DestroyContext();
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout
        << "Vulkan + ImGui + Spout pixel comparison and lifecycle passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
