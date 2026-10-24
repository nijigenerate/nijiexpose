// Windows frame sender: native Vulkan/D3D11 interop or the existing GL sender.
#include "framesend.hpp"
#include "i18n/tr.hpp"
#include "log.hpp"
#ifdef NIJIEXPOSE_VULKAN
#include "render/vulkan/backend.hpp"
#include "render/vulkan/windows_spout.hpp"
#else
#include "nijilive/opengl/runtime.hpp"
#include "spout.hpp"
#include <core/runtime_state.hpp>
#endif
namespace nijiexpose {
namespace {
thread_local bool loadSuccessful = false;
#ifdef NIJIEXPOSE_VULKAN
thread_local std::unique_ptr<vulkan::SpoutBridge> bridge;
#else
thread_local spout::SPOUTHANDLE spHandle = nullptr;
#endif
} // namespace
void insInitFrameSending() {
#ifdef NIJIEXPOSE_VULKAN
  try {
    bridge = std::make_unique<vulkan::SpoutBridge>();
    loadSuccessful = true;
  } catch (const std::exception &e) {
    insLogErr("Spout Vulkan initialization: %s", e.what());
    loadSuccessful = false;
  }
#else
  using namespace spout;
  loadSuccessful = loadSpout2() == Spout2Support::spout2;
  if (loadSuccessful) {
    std::string senderName = "nijiexpose";
    spHandle = spGetSpout();
    int i = 1;
    while (spFindSenderName(spHandle, senderName.data()))
      senderName = "nijiexpose (" + std::to_string(i++) + ")";
    spSetSenderName(spHandle, senderName.c_str());
    spSetSenderFormat(spHandle, 28);
  }
#endif
  if (!loadSuccessful)
    insLogErr(i18n::_("Could not start texture sharing, it will be disabled. "
                      "Is the library missing?"));
  else
    insLogInfo(i18n::_("Frame-sending started successfully!"));
}
void insCleanupFrameSending() {
#ifdef NIJIEXPOSE_VULKAN
  bridge.reset();
#else
  if (spHandle) {
    spout::spReleaseSender(spHandle, 0);
    spHandle = nullptr;
  }
  spout::unloadSpout2();
#endif
  loadSuccessful = false;
}
void insSendFrame() {
  if (!loadSuccessful)
    return;
#ifdef NIJIEXPOSE_VULKAN
  try {
    if (!bridge->send(vulkan::backend().sceneImage()))
      throw std::runtime_error("Spout sender is unavailable");
  } catch (const std::exception &e) {
    insLogErr("Spout Vulkan send: %s", e.what());
    bridge.reset();
    loadSuccessful = false;
  }
#else
  int w, h;
  nicxlive::core::inGetViewport(w, h);
  if (spHandle)
    spout::spSendTexture(spHandle, ogl::oglGetRenderImage(), GL_TEXTURE_2D, w,
                         h, true, ogl::oglGetFramebuffer());
#endif
}
const char *insFrameSenderName() {
#ifdef NIJIEXPOSE_VULKAN
  return bridge ? bridge->name() : "";
#else
  return spHandle ? spout::spGetName(spHandle) : "";
#endif
}
bool insCanSendFrames() { return loadSuccessful; }
} // namespace nijiexpose
