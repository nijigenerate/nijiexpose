#include "window_mode.hpp"
#include <dlfcn.h>

namespace nijiexpose::render {
namespace {
using Display=void;
using XWindow=unsigned long;
using Atom=unsigned long;
struct X11 {
    void* library=dlopen("libX11.so.6",RTLD_NOW|RTLD_LOCAL);
    void* extension=dlopen("libXext.so.6",RTLD_NOW|RTLD_LOCAL);
    template<class T> static T symbol(void* library,const char* name) {
        return library?reinterpret_cast<T>(dlsym(library,name)):nullptr;
    }
    using Intern=Atom(*)(Display*,const char*,int);
    using Change=int(*)(Display*,XWindow,Atom,Atom,int,int,const unsigned char*,int);
    using Delete=int(*)(Display*,XWindow,Atom);
    using Flush=int(*)(Display*);
    using Shape=void(*)(Display*,XWindow,int,int,int,void*,int,int,int);
    using Mask=void(*)(Display*,XWindow,int,int,int,unsigned long,int);
    using Query=int(*)(Display*,int*,int*);
    Intern intern=symbol<Intern>(library,"XInternAtom");
    Change change=symbol<Change>(library,"XChangeProperty");
    Delete remove=symbol<Delete>(library,"XDeleteProperty");
    Flush flush=symbol<Flush>(library,"XFlush");
    Shape shape=symbol<Shape>(extension,"XShapeCombineRectangles");
    Mask mask=symbol<Mask>(extension,"XShapeCombineMask");
    Query query=symbol<Query>(extension,"XShapeQueryVersion");
    ~X11() {if(extension)dlclose(extension);if(library)dlclose(library);}
};
struct Native { Display* display; XWindow window; };
Native nativeWindow(SDL_Window* window) {
    auto props=SDL_GetWindowProperties(window);
    return {SDL_GetPointerProperty(props,SDL_PROP_WINDOW_X11_DISPLAY_POINTER,nullptr),
        static_cast<XWindow>(SDL_GetNumberProperty(props,SDL_PROP_WINDOW_X11_WINDOW_NUMBER,0))};
}
X11& api() {static X11 value;return value;}
constexpr auto passthroughKey="nijiexpose.window.mouse_passthrough";
}
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
        x.shape(native.display,native.window,2,0,0,nullptr,0,0,0);
    } else {
        // Restore the default region, which follows future window resizes.
        x.mask(native.display,native.window,2,0,0,0,0);
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
        const unsigned long zero=0,full=0xffffffffUL;
        x.change(native.display,native.window,bypass,cardinal,32,0,reinterpret_cast<const unsigned char*>(&zero),1);
        x.change(native.display,native.window,opacity,cardinal,32,0,reinterpret_cast<const unsigned char*>(&full),1);
    } else {
        setWindowMousePassthrough(window,false);
        x.remove(native.display,native.window,bypass);
        x.remove(native.display,native.window,opacity);
    }
    x.flush(native.display);
}
}
