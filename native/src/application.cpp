// dlang_archive/source/app.d startup and shutdown, shared by the executable and app interaction tests.
#include "application.hpp"
#include "i18n.hpp"
#include "plugins/package.hpp"
#include "scene/package.hpp"
#include "framesend.hpp"
#include "tracking/expr.hpp"
#include "tracking/tracker.hpp"
#include "panels/factory.hpp"
#include "panels/scene.hpp"
#include "panels/plugin.hpp"
#include "nijilive/opengl/runtime.hpp"
#include "nijilive/opengl/dynamic_composite.hpp"
#include <core/runtime_state.hpp>
#include "nijilive/opengl/blending.hpp"
#include "nijiui/core/app.hpp"
#include "nijiui/core/core.hpp"
#include "nijiui/core/settings.hpp"
namespace nijiexpose {
std::shared_ptr<nijiui::InApplicationWindow> initializeApplication(const std::vector<std::string>& args){
    // D inPanel module constructors register panels before main and locale initialization.
    nijiui::inAddPanel(panels::createTrackingPanel());
    nijiui::inAddPanel(panels::createAnimationsPanel());
    nijiui::inAddPanel(panels::createBlendshapesPanel());
    nijiui::inAddPanel(std::make_shared<panels::ScenePanel>());
    nijiui::inAddPanel(std::make_shared<panels::PluginPanel>());
    nijiui::inSetApplication({"net.nijilive.nijiexpose","nijiexpose","nijiexpose"});
    plugins::insLuaInit();
    nijiui::inInitUI();
    neLocaleInitFromSettings();
    tracking::insInitExpressions();
    auto window=windows::neCreateWindow(args);
    scene::insScene()->init();
    tracking::neInitTracker();
    insInitFrameSending();
    ogl::oglAddBasicLightingPostProcess();
    bool fallback=nijiui::inSettingsGet<bool>("TripleBufferFallback",ogl::nlIsTripleBufferFallbackEnabled());
    ogl::nlSetTripleBufferFallback(fallback);
    return window;
}
void shutdownApplication(){
    tracking::neShutdownTracker();
    tracking::insCleanupExpressions();
    plugins::insLuaUnload();
    insCleanupFrameSending();
    scene::insScene()->cleanup();
    if(SDL_GL_GetCurrentContext()) {
        nicxlive::core::inSetRenderBackend(nullptr);
        ogl::oglCollectDynamicFramebuffers(true);
    }
    nijiui::inSettingsSave();
}
}
