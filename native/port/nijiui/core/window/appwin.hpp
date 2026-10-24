#pragma once
#include "window.hpp"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <string>
namespace nicxlive::core { class ShallowTexture; }
namespace nijiexpose::vulkan { class Context; }
namespace nijiui {
// Windows win32-full configuration of nijiui.core.window.appwin.
// Windows nijiui enables SafeThrottling; UIViewports remains disabled.
class InApplicationWindow : public InWindow {
    SDL_Window* window=nullptr;
    ImGuiContext* ctx=nullptr;
    ImGuiIO* io=nullptr;
    SDL_GLContext glctx=nullptr;
#ifdef NIJIEXPOSE_VULKAN
    std::unique_ptr<nijiexpose::vulkan::Context> vulkanContext;
#endif
    bool done=false,show=true;
    int width_=0,height_=0;
    std::vector<std::string> draggedFiles;
    std::string iniPath;
    int throttlingRate=1;
    unsigned renderThrottlingCount=0;
    unsigned fbo=0,fboTexture=0;
    SDL_Surface* windowIcon=nullptr;
    bool transparencyRetryPending=false;
    bool normalWindowMaximized=false;
    int normalWindowX=0, normalWindowY=0, normalWindowWidth=0, normalWindowHeight=0;
protected:
    SDL_Window* nativeWindow() const { return window; }
    SDL_Surface* nativeIcon() const { return windowIcon; }
    void setBorderlessTransparency(bool enabled);
    std::vector<std::string>& getDraggedFiles() { return draggedFiles; }
    void cleanup();
    void initStyling();
    void initDarkMode();
public:
    InApplicationWindow(std::string title,unsigned width,unsigned height,int throttlingRate=1);
    ~InApplicationWindow() override;
    void onEarlyUpdate() override {}
    void onUpdate() override {}
    virtual void onPlatformUpdate() {}
    virtual bool onPlatformEvent(const SDL_Event&) {return false;}
    virtual void setTransparentMode(bool) {}
    void setThrottlingRate(int rate=1);
    bool shouldProcess() override;
    void update();
    std::string getWindowHandle() { return {}; }
    void focus() override;
    void close() override;
    bool isAlive() override { return !done; }
    int width() override { return width_; }
    int height() override { return height_; }
    void setIcon(const nicxlive::core::ShallowTexture& texture);
    void onResized(int width,int height) override;
};
}
