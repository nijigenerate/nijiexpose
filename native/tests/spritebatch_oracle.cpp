#include "nijiexpose/render/spritebatch.hpp"
#include "nijilive/opengl/backend.hpp"
#include <core/runtime_state.hpp>
#include <SDL3/SDL.h>
#include <bit>
#include <cstdio>
#include <stdexcept>
int main() {
    namespace core=nicxlive::core;
    namespace gl=nijiexpose::ogl;
    if(!SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    auto* window=SDL_CreateWindow("SpriteBatch verification",64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window || !SDL_GL_CreateContext(window)) return 1;
    auto backend=std::make_shared<gl::Backend>(); core::inSetRenderBackend(backend); backend->initializeRenderer();
    core::inSetViewport(64,64); core::inSetClearColor(0,0,0,0);
    std::vector<uint8_t> pixels{255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255};
    auto texture=std::make_shared<core::Texture>(pixels,2,2,4,4,false,false);
    nijiexpose::SpriteBatch batch;
    for(int flip=0;flip<4;++flip) for(float rotation:{0.f,.2f,-.8f}) {
        backend->beginScene();
        batch.draw(texture,{12,18,24,16},{},{2,4},rotation,static_cast<nijiexpose::SpriteFlip>(flip),{.8f,.6f,.7f,.9f});
        batch.flush();
        std::vector<uint8_t> output(64*64*4); core::inDumpViewport(output);
        uint64_t hash=14695981039346656037ULL;
        for(auto b:output) { hash^=b; hash*=1099511628211ULL; }
        std::printf("pixels:%d:%016llx\n",flip,static_cast<unsigned long long>(hash));
        auto readBuffer=reinterpret_cast<PFNGLGETBUFFERSUBDATAPROC>(SDL_GL_GetProcAddress("glGetBufferSubData"));
        if(!readBuffer) return 1;
        float vertices[48]; readBuffer(GL_ARRAY_BUFFER,0,sizeof(vertices),vertices);
        for(float v:vertices) std::printf("%08x",std::bit_cast<uint32_t>(v));
        std::puts("");
    }
}
