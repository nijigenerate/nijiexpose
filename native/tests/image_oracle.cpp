#include "io/image.hpp"
#include "nijilive/opengl/backend.hpp"
#include <core/runtime_state.hpp>
#include <SDL3/SDL.h>
#include <cstdio>
#include <filesystem>
#include <chrono>
int main(int argc,char** argv) {
    namespace core=nicxlive::core;
    namespace gl=nijiexpose::ogl;
    if(argc!=2 || !SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    auto* window=SDL_CreateWindow("Image verification",32,32,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window || !SDL_GL_CreateContext(window)) return 1;
    auto backend=std::make_shared<gl::Backend>(); core::inSetRenderBackend(backend); backend->initializeRenderer();
    glPixelStorei(GL_UNPACK_ALIGNMENT,1); glPixelStorei(GL_PACK_ALIGNMENT,1);
    for(int channels=1;channels<=4;++channels) {
        auto file=std::string(argv[1])+"/channels-"+std::to_string(channels)+".png";
        core::ShallowTexture shallow(file,4);
        std::printf("decode:%d:%d:%d:%d:%d:",channels,shallow.width,shallow.height,shallow.channels,shallow.convChannels);
        for(auto b:shallow.data) std::printf("%02x",b); std::puts("");
        auto puppet=nijiexpose::neLoadModelFromImage(file);
        auto part=std::dynamic_pointer_cast<core::nodes::Part>(puppet->root);
        auto texture=part->textures[0];
        auto data=backend->readTexture(texture->backendId());
        std::printf("crop:%d:%d:%d:%d:",channels,texture->width(),texture->height(),texture->channels());
        for(auto b:data) std::printf("%02x",b); std::puts("");
    }
    const auto unicodeDir=std::filesystem::temp_directory_path()/(std::filesystem::path(u8"nijiexpose-画像検証-").native()+std::filesystem::path(std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())).native());
    std::filesystem::create_directories(unicodeDir);
    const auto unicodeFile=unicodeDir/std::filesystem::path(u8"背景.png");
    std::filesystem::copy_file(std::filesystem::path(argv[1])/"channels-4.png",unicodeFile,std::filesystem::copy_options::overwrite_existing);
    const auto utf8=unicodeFile.u8string();
    auto unicodePuppet=nijiexpose::neLoadModelFromImage(std::string(utf8.begin(),utf8.end()));
    if(!unicodePuppet) return 1;
    std::filesystem::remove(unicodeFile);
    std::filesystem::remove(unicodeDir);
}
