// Windows port of nijiui/core/window/appwin.d.
#include "appwin.hpp"
#include "nijiui/core/app.hpp"
#include "nijiui/core/path.hpp"
#include "nijiui/core/core.hpp"
#include "nijiui/core/font.hpp"
#include "nijiui/panel.hpp"
#include "nijiui/toolwindow.hpp"
#include "nijiui/widgets/dialog.hpp"
#include "nijiui/input.hpp"
#ifndef NIJIEXPOSE_VULKAN
#include "nijilive/opengl/api.hpp"
#include <imgui_impl_opengl3.h>
#endif
#include <imgui_impl_sdl3.h>
#include <core/texture.hpp>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include "render/window_mode.hpp"
#ifdef NIJIEXPOSE_VULKAN
#include "render/vulkan/backend.hpp"
#include "framesend.hpp"
#endif
namespace nijiui {
namespace {
bool isGLLoaded=false;
void fitDecoratedWindowToDisplay(SDL_Window* window) {
    if (SDL_GetWindowFlags(window) & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN)) return;
    SDL_Rect area{};
    if (!SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(window), &area)) return;
    int top=0, left=0, bottom=0, right=0;
    if (!SDL_GetWindowBordersSize(window, &top, &left, &bottom, &right)) return;
    int x=0, y=0, w=0, h=0;
    SDL_GetWindowPosition(window, &x, &y);
    SDL_GetWindowSize(window, &w, &h);
    const int fittedW=std::min(w, std::max(1, area.w-left-right));
    const int fittedH=std::min(h, std::max(1, area.h-top-bottom));
    if (w!=fittedW || h!=fittedH) SDL_SetWindowSize(window, fittedW, fittedH);
    const int fittedX=std::clamp(x, area.x+left, area.x+std::max(left, area.w-fittedW-right));
    const int fittedY=std::clamp(y, area.y+top, area.y+std::max(top, area.h-fittedH-bottom));
    if (x!=fittedX || y!=fittedY) SDL_SetWindowPosition(window, fittedX, fittedY);
}
}
InApplicationWindow::InApplicationWindow(std::string title,unsigned width,unsigned height,int rate)
    : width_(static_cast<int>(width)),height_(static_cast<int>(height)) {
#ifdef NIJIEXPOSE_VULKAN
    SDL_SetHint(SDL_HINT_APP_NAME,inGetApplication().humanName.c_str());
    window=SDL_CreateWindow(title.c_str(),width_,height_,SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE
#ifndef _WIN32
        |SDL_WINDOW_TRANSPARENT
#endif
    );
    if(!window)throw std::runtime_error(SDL_GetError());
    fitDecoratedWindowToDisplay(window);
    vulkanContext=std::make_unique<nijiexpose::vulkan::Context>(window);
    setThrottlingRate(rate);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);
    SDL_SetHint(SDL_HINT_APP_NAME,inGetApplication().humanName.c_str());
    window=SDL_CreateWindow(title.c_str(),width_,height_,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE
#ifndef _WIN32
        |SDL_WINDOW_TRANSPARENT
#endif
    );
    if(!window)throw std::runtime_error(SDL_GetError());
    fitDecoratedWindowToDisplay(window);
    glctx=SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window,glctx);
    setThrottlingRate(rate);
    if(!isGLLoaded) {
        if(!glctx) throw std::runtime_error("No valid OpenGL context was found!");
        nijiexpose::ogl::loadApi();
    }
#endif
    ctx=ImGui::CreateContext();
    io=&ImGui::GetIO();
    io->ConfigFlags|=ImGuiConfigFlags_DockingEnable;
    io->ConfigWindowsResizeFromEdges=true;
    iniPath=(std::filesystem::path(inGetAppConfigPath())/"imgui.ini").string();
    io->IniFilename=iniPath.c_str();
#ifdef NIJIEXPOSE_VULKAN
    ImGui_ImplSDL3_InitForVulkan(window);
    vulkanContext->initializeImGui();
#else
    ImGui_ImplSDL3_InitForOpenGL(window,glctx);
    ImGui_ImplOpenGL3_Init("#version 330");
#endif
    initStyling();
    initDarkMode();
    inInitFonts();
    inInitPanels();
}
InApplicationWindow::~InApplicationWindow() { cleanup(); }
void InApplicationWindow::cleanup() {
#ifdef NIJIEXPOSE_VULKAN
    nijiexpose::insCleanupFrameSending();
    vulkanContext->shutdownImGui();
#else
    ImGui_ImplOpenGL3_Shutdown();
#endif
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(ctx);
#ifdef NIJIEXPOSE_VULKAN
    vulkanContext.reset();
#else
    if(fboTexture)nijiexpose::ogl::glDeleteTextures(1,&fboTexture);
    if(fbo)nijiexpose::ogl::glDeleteFramebuffers(1,&fbo);
    SDL_GL_DestroyContext(glctx);
#endif
    SDL_DestroyWindow(window);
    SDL_DestroySurface(windowIcon);
}
void InApplicationWindow::setIcon(const nicxlive::core::ShallowTexture& texture) {
    const int channels=texture.convChannels>0?texture.convChannels:texture.channels;
    if(texture.width<=0 || texture.height<=0 || (channels!=3 && channels!=4) ||
       texture.data.size()!=size_t(texture.width)*size_t(texture.height)*channels)
        throw std::runtime_error("Invalid window icon texture");
    auto* surface=SDL_CreateSurfaceFrom(texture.width,texture.height,
        channels==4?SDL_PIXELFORMAT_RGBA32:SDL_PIXELFORMAT_RGB24,
        const_cast<uint8_t*>(texture.data.data()),texture.width*channels);
    if(!surface)throw std::runtime_error(SDL_GetError());
    const bool success=SDL_SetWindowIcon(window,surface);
    SDL_DestroySurface(windowIcon);
    windowIcon=SDL_DuplicateSurface(surface);
    SDL_DestroySurface(surface);
    if(!success)throw std::runtime_error(SDL_GetError());
}
void InApplicationWindow::setBorderlessTransparency(bool enabled) {
    if (enabled) {
        normalWindowMaximized=(SDL_GetWindowFlags(window)&SDL_WINDOW_MAXIMIZED)!=0;
        if (normalWindowMaximized) {
            SDL_RestoreWindow(window);
            SDL_SyncWindow(window);
        }
        SDL_GetWindowPosition(window,&normalWindowX,&normalWindowY);
        SDL_GetWindowSize(window,&normalWindowWidth,&normalWindowHeight);
    }
    // A resizable borderless SDL window still has a native sizing frame on Windows.
    SDL_SetWindowResizable(window,!enabled);
    SDL_SetWindowBordered(window,!enabled);
    SDL_SetWindowAlwaysOnTop(window,enabled);
    if (enabled) {
        SDL_Rect area{};
        if (SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(window),&area)) {
            SDL_SetWindowPosition(window,area.x,area.y);
            SDL_SetWindowSize(window,area.w,area.h);
        }
    } else if (normalWindowWidth>0 && normalWindowHeight>0) {
        SDL_SetWindowSize(window,normalWindowWidth,normalWindowHeight);
        SDL_SetWindowPosition(window,normalWindowX,normalWindowY);
    }
    SDL_SyncWindow(window);
    nijiexpose::render::transparentWindow=enabled;
    transparencyRetryPending=enabled;
#ifdef NIJIEXPOSE_VULKAN
    vulkanContext->resize();
#endif
    nijiexpose::render::configureWindowTransparency(window,enabled);
    if (!enabled) fitDecoratedWindowToDisplay(window);
    if (!enabled && normalWindowMaximized) SDL_MaximizeWindow(window);
}
void InApplicationWindow::setThrottlingRate(int rate) {
    throttlingRate=rate;
    if(rate>1 && ctx && io)onResized(static_cast<int>(io->DisplaySize.x),static_cast<int>(io->DisplaySize.y));
#ifdef NIJIEXPOSE_VULKAN
    vulkanContext->setThrottle(rate);
#else
    SDL_GL_SetSwapInterval(rate>0?1:0);
#endif
}
void InApplicationWindow::onResized(int width,int height) {
    transparencyRetryPending=nijiexpose::render::transparentWindow;
#ifdef NIJIEXPOSE_VULKAN
    vulkanContext->resize();
#else
    using namespace nijiexpose::ogl;
    if(fboTexture){nijiexpose::ogl::glDeleteTextures(1,&fboTexture);fboTexture=0;}
    if(fbo){nijiexpose::ogl::glDeleteFramebuffers(1,&fbo);fbo=0;}
    if(throttlingRate>1) {
        nijiexpose::ogl::glGenFramebuffers(1,&fbo);nijiexpose::ogl::glGenTextures(1,&fboTexture);
        nijiexpose::ogl::glBindTexture(GL_TEXTURE_2D,fboTexture);
        nijiexpose::ogl::glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        nijiexpose::ogl::glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        nijiexpose::ogl::glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        nijiexpose::ogl::glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        nijiexpose::ogl::glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,fboTexture,0);
        nijiexpose::ogl::glBindFramebuffer(GL_FRAMEBUFFER,0);
    }
#endif
}
bool InApplicationWindow::shouldProcess() { return window && !done && !(SDL_GetWindowFlags(window)&SDL_WINDOW_MINIMIZED); }
void InApplicationWindow::focus() { SDL_RaiseWindow(window); }
void InApplicationWindow::close() { done=true; onClosed(); }
void InApplicationWindow::update() {
    // Same SafeThrottling path as the Windows D nijiui configuration.
    if(throttlingRate>0 && renderThrottlingCount++ % static_cast<unsigned>(throttlingRate)!=0) {
#ifdef NIJIEXPOSE_VULKAN
        // The compositor retains the last presented image. Skip simulation/UI
        // exactly as SafeThrottling does, without rerendering the same scene.
        SDL_DisplayMode const* mode=SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window));
        SDL_DelayNS(static_cast<Uint64>(1e9/(mode&&mode->refresh_rate>0?mode->refresh_rate:60)));
#else
        using namespace nijiexpose::ogl;
        auto size=io->DisplaySize;
        if(!fbo)onResized(static_cast<int>(size.x),static_cast<int>(size.y));
        nijiexpose::ogl::glBindFramebuffer(GL_READ_FRAMEBUFFER,fbo);
        nijiexpose::ogl::glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
        nijiexpose::ogl::glBlitFramebuffer(0,0,static_cast<int>(size.x),static_cast<int>(size.y),0,0,static_cast<int>(size.x),static_cast<int>(size.y),GL_COLOR_BUFFER_BIT,GL_NEAREST);
        SDL_GL_SwapWindow(window);
#endif
        return;
    }
    inUpdateTime();
    draggedFiles.clear();
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        if(onPlatformEvent(event))continue;
        switch(event.type) {
        case SDL_EVENT_QUIT: close(); break;
        case SDL_EVENT_DROP_FILE:
            draggedFiles.emplace_back(event.drop.data);
            SDL_RaiseWindow(window);
            break;
        default:
            ImGui_ImplSDL3_ProcessEvent(&event);
            if(event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==SDL_GetWindowID(window)) close();
            if(event.type==SDL_EVENT_WINDOW_RESIZED && event.window.windowID==SDL_GetWindowID(window)) {
                SDL_GetWindowSizeInPixels(window,&width_,&height_);
                onResized(width_,height_);
            }
            break;
        }
    }
    onPlatformUpdate();
#ifdef NIJIEXPOSE_VULKAN
    if(!vulkanContext->beginFrame())return;
    ImGui_ImplVulkan_NewFrame();
#else
    SDL_GL_MakeCurrent(window,glctx);
    ImGui_ImplOpenGL3_NewFrame();
#endif
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
#ifdef NIJIEXPOSE_VULKAN
    auto& renderer=nijiexpose::vulkan::backend();
    renderer.resizeViewportTargets(vulkanContext->width(),vulkanContext->height());
#endif
    inInputUpdate();
    if(!draggedFiles.empty() && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
        // D copies the array descriptor. C++ copies a pointer to the same frame-owned array.
        auto* files=&draggedFiles;
        ImGui::SetDragDropPayload("_FILEDROP",&files,sizeof(files));
        ImGui::BeginTooltip();
        for(const auto& file:draggedFiles) ImGui::TextUnformatted(file.c_str());
        ImGui::EndTooltip();
        ImGui::EndDragDropSource();
    }
    ImGui::DockSpaceOverViewport(0,nullptr,ImGuiDockNodeFlags_PassthruCentralNode);
    onUpdate();
    if(showUI) {
        inUpdatePanels();
        inUpdateToolWindows();
        uiImRenderDialogs();
        for(auto& win:inWindowListGet()) { win->onEarlyUpdate(); win->onUpdate(); }
    }
    ImGui::Render();
#ifdef NIJIEXPOSE_VULKAN
    onEarlyUpdate();
    renderer.render();
    vulkanContext->present(ImGui::GetDrawData(),renderer.sceneImage());
    nijiexpose::insSendFrame();
#else
    // Settings can create/destroy a tracker preview renderer during this frame.
    SDL_GL_MakeCurrent(window,glctx);
    nijiexpose::ogl::glViewport(0,0,static_cast<int>(io->DisplaySize.x),static_cast<int>(io->DisplaySize.y));
    nijiexpose::ogl::glClearColor(0,0,0,0);
    nijiexpose::ogl::glClear(GL_COLOR_BUFFER_BIT);
    onEarlyUpdate();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if(throttlingRate>1) {
        using namespace nijiexpose::ogl;
        auto size=io->DisplaySize;
        if(!fbo)onResized(static_cast<int>(size.x),static_cast<int>(size.y));
        nijiexpose::ogl::glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
        nijiexpose::ogl::glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fbo);
        nijiexpose::ogl::glBlitFramebuffer(0,0,static_cast<int>(size.x),static_cast<int>(size.y),0,0,static_cast<int>(size.x),static_cast<int>(size.y),GL_COLOR_BUFFER_BIT,GL_NEAREST);
    }
#ifdef NIJIEXPOSE_DYNAMIC_MOUSE_PASSTHROUGH
    if (nijiexpose::render::transparentWindow) {
        int w=0,h=0,x=0,y=0;
        SDL_GetWindowSizeInPixels(window,&w,&h);
        const bool inside=nijiexpose::render::windowPointerPixel(window,w,h,x,y);
        unsigned char pixel[4]={0,0,0,255};
        if (inside) {
            // nijimi samples the final alpha at the pointer, including UI.
            nijiexpose::ogl::glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
            glReadPixels(x,h-1-y,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
        }
        const bool dragging=SDL_GetGlobalMouseState(nullptr,nullptr)&SDL_BUTTON_LMASK;
        nijiexpose::render::setWindowMousePassthrough(window,(!inside || pixel[3]<8) && !dragging);
    } else {
        nijiexpose::render::setWindowMousePassthrough(window,false);
    }
#endif
    SDL_GL_SwapWindow(window);
#endif
    // Like nijimi, apply again after the first present for native layers created lazily.
    if(transparencyRetryPending) {
        nijiexpose::render::configureWindowTransparency(window,true);
        transparencyRetryPending=false;
    }
    uiImCleanupDialogs();
}
}

namespace nijiui {
void InApplicationWindow::initStyling() {
        auto* style = &ImGui::GetStyle();
        
        style->ChildBorderSize = 1;
        style->PopupBorderSize = 1;
        style->FrameBorderSize = 1;
        style->TabBorderSize = 1;

        style->WindowRounding = 4;
        style->ChildRounding = 0;
        style->FrameRounding = 3;
        style->PopupRounding = 6;
        style->ScrollbarRounding = 18;
        style->GrabRounding = 3;
        style->LogSliderDeadzone = 6;
        style->TabRounding = 6;

        style->IndentSpacing = 10;
        style->ItemSpacing.y = 3;
        style->FramePadding.y = 4;

        style->GrabMinSize = 13;
        style->ScrollbarSize = 14;
        style->ChildBorderSize = 1;
    }
void InApplicationWindow::initDarkMode() {
        auto* style = &ImGui::GetStyle();
        
        style->Colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style->Colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        style->Colors[ImGuiCol_WindowBg]               = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        style->Colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style->Colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        style->Colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.16f);
        style->Colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.16f);
        style->Colors[ImGuiCol_FrameBg]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        style->Colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.15f, 0.15f, 0.15f, 0.40f);
        style->Colors[ImGuiCol_FrameBgActive]          = ImVec4(0.22f, 0.22f, 0.22f, 0.67f);
        style->Colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        style->Colors[ImGuiCol_TitleBgActive]          = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        style->Colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        style->Colors[ImGuiCol_MenuBarBg]              = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
        style->Colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        style->Colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style->Colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        style->Colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        style->Colors[ImGuiCol_CheckMark]              = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        style->Colors[ImGuiCol_SliderGrab]             = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style->Colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style->Colors[ImGuiCol_Button]                 = ImVec4(0.39f, 0.39f, 0.39f, 0.40f);
        style->Colors[ImGuiCol_ButtonHovered]          = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
        style->Colors[ImGuiCol_ButtonActive]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        style->Colors[ImGuiCol_Header]                 = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style->Colors[ImGuiCol_HeaderHovered]          = ImVec4(0.28f, 0.28f, 0.28f, 0.80f);
        style->Colors[ImGuiCol_HeaderActive]           = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
        style->Colors[ImGuiCol_Separator]              = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        style->Colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.29f, 0.29f, 0.29f, 0.78f);
        style->Colors[ImGuiCol_SeparatorActive]        = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
        style->Colors[ImGuiCol_ResizeGrip]             = ImVec4(0.35f, 0.35f, 0.35f, 0.00f);
        style->Colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.40f, 0.40f, 0.40f, 0.00f);
        style->Colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.55f, 0.55f, 0.56f, 0.00f);
        style->Colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        style->Colors[ImGuiCol_TabHovered]             = ImVec4(0.34f, 0.34f, 0.34f, 0.80f);
        style->Colors[ImGuiCol_TabSelected]              = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style->Colors[ImGuiCol_TabDimmed]           = ImVec4(0.14f, 0.14f, 0.14f, 0.97f);
        style->Colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        style->Colors[ImGuiCol_DockingPreview]         = ImVec4(0.62f, 0.68f, 0.75f, 0.70f);
        style->Colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        style->Colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        style->Colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        style->Colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style->Colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style->Colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        style->Colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        style->Colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        style->Colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style->Colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        style->Colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        style->Colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        style->Colors[ImGuiCol_NavCursor]           = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
        style->Colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        style->Colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        style->Colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        style->FrameBorderSize = 1;
        style->TabBorderSize = 1;
    }
}
