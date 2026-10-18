// source/app.d startup and shutdown, shared by the executable and app interaction tests.
#include "application.hpp"
#include "nijiexpose/i18n.hpp"
#include "nijiexpose/plugins/package.hpp"
#include "nijiexpose/scene/package.hpp"
#include "nijiexpose/framesend.hpp"
#include "nijiexpose/tracking/expr.hpp"
#include "nijiexpose/tracking/tracker.hpp"
#include "nijiexpose/panels/factory.hpp"
#include "nijiexpose/panels/scene.hpp"
#include "nijiexpose/panels/plugin.hpp"
#include "nijilive/opengl/runtime.hpp"
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
    tracking::neInitTracker();
    auto window=windows::neCreateWindow(args);
    scene::insScene()->init();
    insInitFrameSending();
    ogl::oglAddBasicLightingPostProcess();
    bool fallback=nijiui::inSettingsGet<bool>("TripleBufferFallback",ogl::nlIsTripleBufferFallbackEnabled());
    ogl::nlSetTripleBufferFallback(fallback);
    return window;
}
void shutdownApplication(){
    tracking::insCleanupExpressions();
    plugins::insLuaUnload();
    insCleanupFrameSending();
    scene::insScene()->cleanup();
    nijiui::inSettingsSave();
    tracking::neShutdownTracker();
}
}
