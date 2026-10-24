#include "window_mode.hpp"
#include <stdexcept>
#include <windows.h>
#include <dwmapi.h>
#include <commctrl.h>
namespace nijiexpose::render {
namespace {
LRESULT CALLBACK transparentFrameProc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR) {
    // Layered mouse passthrough changes activation. Do not let DefWindowProc
    // repaint a classic caption on the borderless DWM surface on activation loss.
    if (message==WM_NCACTIVATE) return TRUE;
    if (message==WM_NCPAINT) return 0;
    if (message==WM_NCDESTROY) RemoveWindowSubclass(hwnd,transparentFrameProc,id);
    return DefSubclassProc(hwnd,message,wparam,lparam);
}
}
void setWindowMousePassthrough(SDL_Window* window,bool enabled) {
#ifdef NIJIEXPOSE_VULKAN
    auto hwnd=static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window),SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr));
    if (!hwnd) return;
    const auto style=GetWindowLongPtrW(hwnd,GWL_EXSTYLE);
    const LONG_PTR flags=WS_EX_TRANSPARENT|WS_EX_LAYERED;
    const auto next=enabled ? style|flags : style&~flags;
    if (next==style) return;
    // nijimi's DWM input policy. OpenGL uses its native color-key hit testing.
    SetWindowLongPtrW(hwnd,GWL_EXSTYLE,next);
    if (enabled) SetLayeredWindowAttributes(hwnd,0,255,LWA_ALPHA);
    SetWindowPos(hwnd,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
#else
    (void)window;(void)enabled;
#endif
}
void configureWindowTransparency(SDL_Window* window,bool enabled) {
    auto hwnd=static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window),SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr));
    if(!hwnd)throw std::runtime_error("Missing native window handle");
    auto style=GetWindowLongPtrW(hwnd,GWL_EXSTYLE);
    if (enabled) {
        if (!SetWindowSubclass(hwnd,transparentFrameProc,1,0))
            throw std::runtime_error("Cannot suppress transparent window caption painting");
    } else {
        RemoveWindowSubclass(hwnd,transparentFrameProc,1);
    }
#ifdef NIJIEXPOSE_VULKAN
    // Same DWM composition path as nijimi/uilib/windows.d.
    SetWindowLongPtrW(hwnd,GWL_EXSTYLE,style&~(WS_EX_LAYERED|WS_EX_TRANSPARENT));
    MARGINS margins=enabled?MARGINS{-1,-1,-1,-1}:MARGINS{};
    if(FAILED(DwmExtendFrameIntoClientArea(hwnd,&margins)))throw std::runtime_error("DWM transparency is unavailable");
    DWM_BLURBEHIND blur{};blur.dwFlags=DWM_BB_ENABLE;blur.fEnable=enabled;
    if(FAILED(DwmEnableBlurBehindWindow(hwnd,&blur)))throw std::runtime_error("DWM composition is unavailable");
#else
    SetWindowLongPtrW(hwnd,GWL_EXSTYLE,enabled?style|WS_EX_LAYERED:style&~WS_EX_LAYERED);
    if(enabled&&!SetLayeredWindowAttributes(hwnd,RGB(255,0,255),255,LWA_COLORKEY))throw std::runtime_error("Window color-key transparency failed");
#endif
    // Refresh the nonclient area after both SDL decoration and layered/DWM
    // changes; otherwise the old caption can remain painted after a mode switch.
    SetWindowPos(hwnd,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
    RedrawWindow(hwnd,nullptr,nullptr,RDW_INVALIDATE|RDW_FRAME|RDW_UPDATENOW);

}
}
