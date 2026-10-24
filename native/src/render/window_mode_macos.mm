#include "window_mode.hpp"
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include <vector>
#include <stdexcept>

namespace nijiexpose::render {
namespace {
constexpr auto stateKey="nijiexpose.window.transparency.macos";
NSWindow* nativeWindow(SDL_Window* window) {
    return (__bridge NSWindow*)SDL_GetPointerProperty(SDL_GetWindowProperties(window),SDL_PROP_WINDOW_COCOA_WINDOW_POINTER,nullptr);
}
struct LayerState {
    __strong CALayer* layer;
    bool opaque;
    __strong id background;
};
struct ViewState { __strong NSView* view; bool wantsLayer; };
struct GLState { __strong NSOpenGLContext* context; GLint opacity; };
struct State {
    bool opaque, shadow;
    __strong NSColor* background;
    std::vector<LayerState> layers;
    std::vector<ViewState> views;
    std::vector<GLState> contexts;
};
void applyLayer(CALayer* layer,State& state) {
    if (!layer) return;
    bool known=false;
    for (const auto& saved:state.layers) if (saved.layer==layer) known=true;
    if (!known) state.layers.push_back({layer,bool(layer.opaque),(__bridge id)layer.backgroundColor});
    layer.opaque=NO;
    layer.backgroundColor=NSColor.clearColor.CGColor;
    for (CALayer* child in layer.sublayers) applyLayer(child,state);
}
void applyView(NSView* view,State& state) {
    if (!view) return;
    bool known=false;
    for (const auto& saved:state.views) if (saved.view==view) known=true;
    if (!known) state.views.push_back({view,bool(view.wantsLayer)});
    view.wantsLayer=YES;
    applyLayer(view.layer,state);
#ifndef NIJIEXPOSE_VULKAN
    if ([view isKindOfClass:NSOpenGLView.class]) {
        NSOpenGLContext* context=((NSOpenGLView*)view).openGLContext;
        if (context) {
            bool saved=false;
            for (const auto& value:state.contexts) if (value.context==context) saved=true;
            if (!saved) {
                GLint opacity=1;
                [context getValues:&opacity forParameter:NSOpenGLCPSurfaceOpacity];
                state.contexts.push_back({context,opacity});
            }
            GLint opacity=0;
            [context setValues:&opacity forParameter:NSOpenGLCPSurfaceOpacity];
        }
    }
#endif
    for (NSView* child in view.subviews) applyView(child,state);
}
}
void setWindowMousePassthrough(SDL_Window* window,bool enabled) {
    NSWindow* native=nativeWindow(window);
    if (native && native.ignoresMouseEvents!=enabled) native.ignoresMouseEvents=enabled;
}
void configureWindowTransparency(SDL_Window* window,bool enabled) {
    @autoreleasepool {
        NSWindow* native=nativeWindow(window);
        if (!native) throw std::runtime_error("Missing Cocoa window");
        auto props=SDL_GetWindowProperties(window);
        auto* state=static_cast<State*>(SDL_GetPointerProperty(props,stateKey,nullptr));
        if (enabled) {
            if (!state) {
                state=new State{bool(native.opaque),bool(native.hasShadow),native.backgroundColor,{},{},{}};
                if (!SDL_SetPointerPropertyWithCleanup(props,stateKey,state,
                    [](void*,void* value){delete static_cast<State*>(value);},nullptr)) {
                    // SDL invokes the cleanup callback if setting the property fails.
                    throw std::runtime_error(SDL_GetError());
                }
            }
            // nijimi: make the NSWindow, view/layer tree and GL surface non-opaque.
            native.opaque=NO;
            native.backgroundColor=NSColor.clearColor;
            native.hasShadow=NO;
            // Cocoa drag destinations are separate from mouse hit testing.
            // The desktop overlay must not claim a file being dragged to an app below it.
            [native unregisterDraggedTypes];
            applyView(native.contentView,*state);
        } else {
            native.ignoresMouseEvents=NO;
            if (!state) return;
            native.opaque=state->opaque;
            native.backgroundColor=state->background;
            native.hasShadow=state->shadow;
            // Restore the same types SDL's Cocoa_AcceptDragAndDrop registers.
            if (SDL_EventEnabled(SDL_EVENT_DROP_FILE) || SDL_EventEnabled(SDL_EVENT_DROP_TEXT))
                [native registerForDraggedTypes:@[NSPasteboardTypeFileURL,NSPasteboardTypeString]];
            for (const auto& saved:state->contexts) {
                GLint opacity=saved.opacity;
                [saved.context setValues:&opacity forParameter:NSOpenGLCPSurfaceOpacity];
            }
            for (const auto& saved:state->layers) {
                saved.layer.opaque=saved.opaque;
                saved.layer.backgroundColor=(__bridge CGColorRef)saved.background;
            }
            for (auto it=state->views.rbegin();it!=state->views.rend();++it) it->view.wantsLayer=it->wantsLayer;
            SDL_ClearProperty(props,stateKey);
        }
    }
}
}
