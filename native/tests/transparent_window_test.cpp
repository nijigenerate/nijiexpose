#include "application.hpp"
#include "scene/package.hpp"
#include "render/window_mode.hpp"
#include <imgui_internal.h>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <oleidl.h>
#ifdef NIJIEXPOSE_VULKAN
#include "render/vulkan/backend.hpp"
#endif
#endif

namespace {
#ifdef _WIN32
// Probe OLE registration without reading SDL or Windows private structures.
struct DropProbe final : IDropTarget {
    ULONG refs=1;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** value) override {
        if(!value)return E_POINTER;
        *value=nullptr;
        if(iid!=IID_IUnknown && iid!=IID_IDropTarget)return E_NOINTERFACE;
        *value=static_cast<IDropTarget*>(this);AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { return --refs; }
    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject*,DWORD,POINTL,DWORD*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE DragOver(DWORD,POINTL,DWORD*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE DragLeave() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE Drop(IDataObject*,DWORD,POINTL,DWORD*) override { return E_NOTIMPL; }
};
#endif
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: transparent_window_test model.inp");
        auto window = nijiexpose::initializeApplication({argv[1]});
        require(nijiexpose::scene::insScene()->sceneItems.size() == 1,
                "Real model was not loaded");
        int count = 0;
        auto windows = SDL_GetWindows(&count);
        require(windows && count > 0, "No application window");
        auto native = windows[0];
        SDL_free(windows);
        auto requireDropRegistration=[&](bool expected) {
#ifdef _WIN32
            auto hwnd=static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(native),SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr));
            DropProbe probe;
            const auto result=RegisterDragDrop(hwnd,&probe);
            if(result==S_OK) require(RevokeDragDrop(hwnd)==S_OK,"Cannot revoke probe drop target");
            require(result==S_OK || result==DRAGDROP_E_ALREADYREGISTERED,"Cannot probe OLE drop registration");
            require((result==DRAGDROP_E_ALREADYREGISTERED)==expected,
                    expected?"Normal mode lost its drop target":"Transparent overlay still owns an OLE drop target");
#endif
        };
        auto requireNativeFrame = [&](bool transparent) {
#ifdef _WIN32
            auto hwnd=static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(native),SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr));
            const auto style=GetWindowLongPtrW(hwnd,GWL_STYLE);
            RECT outer{}, client{};
            POINT origin{};
            require(GetWindowRect(hwnd,&outer) && GetClientRect(hwnd,&client) && ClientToScreen(hwnd,&origin), "Cannot measure native window frame");
            if (transparent) {
                require(!(style & (WS_CAPTION|WS_THICKFRAME)), "Native caption/sizing style remains in transparent mode");
                require(origin.x==outer.left && origin.y==outer.top &&
                        client.right==outer.right-outer.left && client.bottom==outer.bottom-outer.top,
                        "Transparent outer/client rectangles differ");
            } else {
                require((style & WS_CAPTION)==WS_CAPTION && (style & WS_THICKFRAME), "Normal native caption/sizing style was not restored");
                require(origin.y>outer.top, "Normal caption has no nonclient area");
            }
#endif
        };
        auto frames = [&] {
            for (int i = 0; i < 12; ++i) {
                window->update();
                require(window->isAlive(), "Application closed during mode change");
            }
        };
        auto requireVisibleFrame = [&] {
            SDL_Rect area{};
            int x=0, y=0, w=0, h=0, top=0, left=0, bottom=0, right=0;
            require(SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(native), &area), SDL_GetError());
            require(SDL_GetWindowBordersSize(native, &top, &left, &bottom, &right), SDL_GetError());
            SDL_GetWindowPosition(native, &x, &y);
            SDL_GetWindowSize(native, &w, &h);
            if (SDL_GetWindowFlags(native)&SDL_WINDOW_MAXIMIZED) {
                // Windows places the invisible resize border outside the work
                // area when maximized. The client and caption remain inside it.
                require(x>=area.x && y>=area.y && x+w<=area.x+area.w && y+h<=area.y+area.h,
                        "Maximized client is clipped by the work area");
                return;
            }
            require(x-left >= area.x && y-top >= area.y &&
                    x+w+right <= area.x+area.w && y+h+bottom <= area.y+area.h,
                    "Decorated window is clipped by the display work area");
        };
        frames();
        requireVisibleFrame();
        requireNativeFrame(false);
        requireDropRegistration(true);
        for (int cycle = 0; cycle < 3; ++cycle) {
            if (cycle==1) {
                require(SDL_MaximizeWindow(native), SDL_GetError());
                frames();
            }
            window->setTransparentMode(true);
            frames();
            require(nijiexpose::render::transparentWindow, "Transparent mode failed");
            require(SDL_GetWindowFlags(native) & SDL_WINDOW_BORDERLESS,
                    "Transparent window has a title bar");
            require(!(SDL_GetWindowFlags(native) & SDL_WINDOW_RESIZABLE),
                    "Transparent window retains a resize frame");
            int top=0, left=0, bottom=0, right=0;
            require(SDL_GetWindowBordersSize(native,&top,&left,&bottom,&right),SDL_GetError());
            require(top==0 && left==0 && bottom==0 && right==0,
                    "Transparent window has native borders");
            requireNativeFrame(true);
            requireDropRegistration(false);
            SDL_Rect transparentArea{};
            int tx=0,ty=0,tw=0,th=0;
            SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(native),&transparentArea);
            SDL_GetWindowPosition(native,&tx,&ty);
            SDL_GetWindowSize(native,&tw,&th);
            require(tx==transparentArea.x && ty==transparentArea.y && tw==transparentArea.w && th==transparentArea.h,
                    "Transparent window does not cover the work area");
#if defined(_WIN32) && defined(NIJIEXPOSE_VULKAN)
            if (cycle==0) {
                auto& context=nijiexpose::vulkan::context();
                auto& scene=nijiexpose::vulkan::backend().sceneImage();
                // Full readback is test-only: locate one opaque model pixel and
                // one empty pixel, then exercise the production 1-pixel probe.
                auto pixels=context.read(scene);
                int opaqueX=-1, opaqueY=-1, best=INT_MAX;
                for (int y=0; y<static_cast<int>(scene.height); ++y)
                    for (int x=0; x<static_cast<int>(scene.width); ++x)
                        if (pixels[(size_t(y)*scene.width+x)*4+3]>200) {
                            int distance=std::abs(x-int(scene.width/2))+std::abs(y-int(scene.height/2));
                            if (distance<best) {best=distance;opaqueX=x;opaqueY=y;}
                        }
                require(opaqueX>=0,"No opaque model pixel for hit testing");
                require(pixels[3]<8,"Expected transparent corner for hit testing");
                auto hwnd=static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(native),SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr));
                auto checkPointer=[&](int px,int py,bool passthrough) {
                    float gx=0,gy=0;
                    SDL_GetGlobalMouseState(&gx,&gy);
                    int w=0,h=0;
                    SDL_GetWindowSize(native,&w,&h);
                    require(SDL_SetWindowPosition(native,int(gx)-px*w/int(scene.width),int(gy)-py*h/int(scene.height)),SDL_GetError());
                    frames();
                    const bool actual=(GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT)!=0;
                    require(actual==passthrough,"Final alpha did not update native mouse passthrough");
                    requireNativeFrame(true);
                };
                checkPointer(0,0,true);
                checkPointer(opaqueX,opaqueY,false);
                checkPointer(0,0,true);
                std::cout << "Transparent -> opaque -> transparent pixel input policy passed\n";
            }
#endif
            auto nav = ImGui::FindWindowByName("nijikan_shell_nav###nijikan_shell_nav");
            require(!nav || !nav->Active, "Navigation remained visible in transparent mode");
            require(SDL_SetWindowSize(native, 900 + cycle * 40, 700 + cycle * 20), SDL_GetError());
            SDL_Rect area{};
            require(SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(native), &area), SDL_GetError());
            // A borderless window may touch the top edge. Restoring decorations
            // must make room for the caption instead of placing it offscreen.
            require(SDL_SetWindowPosition(native, area.x, area.y), SDL_GetError());
            frames();
            window->setTransparentMode(false);
            frames();
            require(!nijiexpose::render::transparentWindow, "Normal mode was not restored");
            require(!(SDL_GetWindowFlags(native) & SDL_WINDOW_BORDERLESS),
                    "Title bar was not restored");
            requireVisibleFrame();
            requireNativeFrame(false);
            requireDropRegistration(true);
            require(bool(SDL_GetWindowFlags(native)&SDL_WINDOW_MAXIMIZED)==(cycle>=1),
                    "Normal maximized state was not restored");
            require(SDL_GetWindowFlags(native) & SDL_WINDOW_RESIZABLE,
                    "Normal window resizing was not restored");
            nav = ImGui::FindWindowByName("nijikan_shell_nav###nijikan_shell_nav");
            require(nav && nav->Active, "Navigation was not restored");
            require(nijiexpose::scene::insScene()->sceneItems.size() == 1,
                    "Mode change lost the model");
        }
#ifdef _WIN32
        // Preserve independently disabled file/text events, including across
        // repeated configuration (resize) and destruction while transparent.
        for (int flags=0; flags<4; ++flags) {
            SDL_SetEventEnabled(SDL_EVENT_DROP_FILE,(flags&1)!=0);
            SDL_SetEventEnabled(SDL_EVENT_DROP_TEXT,(flags&2)!=0);
            window->setTransparentMode(true);
            frames();
            nijiexpose::render::configureWindowTransparency(native,true);
            requireDropRegistration(false);
            window->setTransparentMode(false);
            frames();
            require(SDL_EventEnabled(SDL_EVENT_DROP_FILE)==bool(flags&1) &&
                    SDL_EventEnabled(SDL_EVENT_DROP_TEXT)==bool(flags&2),
                    "Mode switch changed the original drop event policy");
            requireDropRegistration(flags!=0);
        }
        window->setTransparentMode(true);
        frames();
        requireDropRegistration(false);
#endif
        window->close();
        nijiexpose::shutdownApplication();
        window.reset();
#ifdef _WIN32
        require(SDL_EventEnabled(SDL_EVENT_DROP_FILE) && SDL_EventEnabled(SDL_EVENT_DROP_TEXT),
                "Closing the transparent window did not restore drop events");
        std::cout << "OLE drop target removal/restoration, all event policies, transparent shutdown passed\n";
#endif
        std::cout << "Real model: startup transparency, three mode cycles, resize, navigation restoration passed\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
