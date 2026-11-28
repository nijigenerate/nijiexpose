#include "window_mode.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/shape.h>
#include <dlfcn.h>
#include <cstring>
#include <string>

namespace nijiexpose::render {
namespace {
struct X11 {
    void* library=dlopen("libX11.so.6",RTLD_NOW|RTLD_LOCAL);
    void* extension=dlopen("libXext.so.6",RTLD_NOW|RTLD_LOCAL);
#ifdef NIJIEXPOSE_VULKAN
    void* render=dlopen("libXrender.so.1",RTLD_NOW|RTLD_LOCAL);
#endif
    template<class T> static T symbol(void* library,const char* name) {
        return library?reinterpret_cast<T>(dlsym(library,name)):nullptr;
    }
    decltype(&XInternAtom) intern=symbol<decltype(intern)>(library,"XInternAtom");
    decltype(&XChangeProperty) change=symbol<decltype(change)>(library,"XChangeProperty");
    decltype(&XDeleteProperty) remove=symbol<decltype(remove)>(library,"XDeleteProperty");
    decltype(&XFlush) flush=symbol<decltype(flush)>(library,"XFlush");
    decltype(&XShapeCombineRectangles) shape=symbol<decltype(shape)>(extension,"XShapeCombineRectangles");
    decltype(&XShapeCombineMask) mask=symbol<decltype(mask)>(extension,"XShapeCombineMask");
    decltype(&XShapeQueryVersion) query=symbol<decltype(query)>(extension,"XShapeQueryVersion");
#ifdef NIJIEXPOSE_VULKAN
    decltype(&XOpenDisplay) openDisplay=symbol<decltype(openDisplay)>(library,"XOpenDisplay");
    decltype(&XCloseDisplay) closeDisplay=symbol<decltype(closeDisplay)>(library,"XCloseDisplay");
    decltype(&XGetVisualInfo) getVisualInfo=symbol<decltype(getVisualInfo)>(library,"XGetVisualInfo");
    decltype(&XFree) free=symbol<decltype(free)>(library,"XFree");
    decltype(&XRenderFindVisualFormat) findVisualFormat=symbol<decltype(findVisualFormat)>(render,"XRenderFindVisualFormat");
#endif
    ~X11() {
#ifdef NIJIEXPOSE_VULKAN
        if(render)dlclose(render);
#endif
        if(extension)dlclose(extension);
        if(library)dlclose(library);
    }
};
struct Native { Display* display; Window window; };
Native nativeWindow(SDL_Window* window) {
    auto props=SDL_GetWindowProperties(window);
    return {static_cast<Display*>(SDL_GetPointerProperty(props,SDL_PROP_WINDOW_X11_DISPLAY_POINTER,nullptr)),
        static_cast<Window>(SDL_GetNumberProperty(props,SDL_PROP_WINDOW_X11_WINDOW_NUMBER,0))};
}
X11& api() {static X11 value;return value;}
constexpr auto passthroughKey="nijiexpose.window.mouse_passthrough";

#ifdef NIJIEXPOSE_VULKAN
VisualID findAlphaVisual() {
    auto& x=api();
    if (!x.openDisplay || !x.closeDisplay || !x.getVisualInfo || !x.free || !x.findVisualFormat) return 0;
    auto* display=x.openDisplay(nullptr);
    if (!display) return 0;

    XVisualInfo criteria{};
    criteria.screen=DefaultScreen(display);
    criteria.depth=32;
    criteria.c_class=TrueColor;
    int count=0;
    auto* visuals=x.getVisualInfo(display,VisualScreenMask|VisualDepthMask|VisualClassMask,&criteria,&count);
    VisualID id=0;
    for (int i=0; i<count; ++i) {
        const auto* format=x.findVisualFormat(display,visuals[i].visual);
        if (format && format->type==PictTypeDirect && format->direct.alphaMask==0xff) {
            id=visuals[i].visualid;
            break;
        }
    }
    if (visuals) x.free(visuals);
    x.closeDisplay(display);
    return id;
}
#endif
}

#ifdef NIJIEXPOSE_VULKAN
SDL_Window* createLinuxVulkanWindow(const char* title,int width,int height,SDL_WindowFlags flags) {
    constexpr auto hint=SDL_HINT_VIDEO_X11_WINDOW_VISUALID;
    const auto* driver=SDL_GetCurrentVideoDriver();
    if (!driver || std::strcmp(driver,"x11")!=0 || SDL_GetHint(hint))
        return SDL_CreateWindow(title,width,height,flags);

    // SDL's X11 backend picks an alpha visual for OpenGL only. Vulkan needs
    // the visual at window creation, including when starting in normal mode.
    const auto id=findAlphaVisual();
    if (!id || !SDL_SetHint(hint,std::to_string(id).c_str()))
        return SDL_CreateWindow(title,width,height,flags);
    auto* window=SDL_CreateWindow(title,width,height,flags);
    SDL_ResetHint(hint);
    return window;
}
#endif

void setWindowMousePassthrough(SDL_Window* window,bool enabled) {
    const auto native=nativeWindow(window);
    if (!native.display || !native.window) return; // nijimi supports X11 only.
    const auto props=SDL_GetWindowProperties(window);
    if (SDL_GetBooleanProperty(props,passthroughKey,false)==enabled) return;
    auto& x=api();
    int major=0,minor=0;
    if (!x.shape || !x.mask || !x.flush || !x.query || !x.query(native.display,&major,&minor) ||
        major<1 || (major==1 && minor<1)) return;
    if (enabled) {
        // Empty ShapeInput region, as in nijimi.
        x.shape(native.display,native.window,ShapeInput,0,0,nullptr,0,ShapeSet,Unsorted);
    } else {
        // Restore the default region, which follows future window resizes.
        x.mask(native.display,native.window,ShapeInput,0,0,None,ShapeSet);
    }
    x.flush(native.display);
    SDL_SetBooleanProperty(props,passthroughKey,enabled);
}
void configureWindowTransparency(SDL_Window* window,bool enabled) {
    const auto native=nativeWindow(window);
    if (!native.display || !native.window) {
        if (enabled) SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,"nijimi-style desktop input passthrough requires X11; Wayland is unsupported");
        return;
    }
    auto& x=api();
    if (!x.intern || !x.change || !x.remove || !x.flush) return;
    const auto cardinal=x.intern(native.display,"CARDINAL",0);
    const auto bypass=x.intern(native.display,"_NET_WM_BYPASS_COMPOSITOR",0);
    const auto opacity=x.intern(native.display,"_NET_WM_WINDOW_OPACITY",0);
    if (enabled) {
        // Xlib format 32 consumes native longs, including on 64-bit systems.
        const unsigned long keepCompositing=2,full=0xffffffffUL;
        x.change(native.display,native.window,bypass,cardinal,32,PropModeReplace,reinterpret_cast<const unsigned char*>(&keepCompositing),1);
        x.change(native.display,native.window,opacity,cardinal,32,PropModeReplace,reinterpret_cast<const unsigned char*>(&full),1);
    } else {
        setWindowMousePassthrough(window,false);
        x.remove(native.display,native.window,bypass);
        x.remove(native.display,native.window,opacity);
    }
    x.flush(native.display);
}
}
