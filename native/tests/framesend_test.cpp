#include "framesend.hpp"
#include "spout.hpp"
#include "nijilive/opengl/backend.hpp"
#include <core/runtime_state.hpp>
#include <SDL3/SDL.h>
#include <iostream>
#include <stdexcept>
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
int main() {
    try {
        check(SDL_Init(SDL_INIT_VIDEO),SDL_GetError());
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
        auto* window=SDL_CreateWindow("Spout verification",32,32,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
        check(window,SDL_GetError()); auto context=SDL_GL_CreateContext(window); check(context,SDL_GetError());
        auto backend=std::make_shared<nijiexpose::ogl::Backend>(); nicxlive::core::inSetRenderBackend(backend);
        backend->initializeRenderer(); nicxlive::core::inSetViewport(32,32);
        nicxlive::core::inSetClearColor(.25f,.5f,.75f,1); backend->beginScene(); backend->endScene();
        nijiexpose::insInitFrameSending();
        check(nijiexpose::insCanSendFrames(),"Spout did not initialize");
        nijiexpose::insSendFrame(); nijiexpose::insSendFrame();
        auto observer=nijiexpose::spout::spGetSpout();
        char name[]="nijiexpose";
        check(nijiexpose::spout::spFindSenderName(observer,name),"Published Spout sender missing");
        uint32_t width=0,height=0; HANDLE handle=nullptr; DWORD format=0;
        check(nijiexpose::spout::spGetSenderInfo(observer,name,width,height,handle,format),"Cannot read published sender");
        check(width==32 && height==32 && handle!=nullptr && format==28,"Shared texture dimensions or DXGI format differs");
        nijiexpose::insCleanupFrameSending();
        check(!nijiexpose::spout::isSpout2Loaded(),"Spout DLL was not unloaded");
        SDL_GL_DestroyContext(context); SDL_DestroyWindow(window); SDL_Quit();
        std::cout<<"Spout: real GPU texture published at 32x32, RGBA8; sender cleanup passed\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
