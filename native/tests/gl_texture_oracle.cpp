#include "nijilive/opengl/resources.hpp"
#include <SDL3/SDL.h>
#include <cstdio>
#include <vector>
namespace gl = nijiexpose::ogl;
int main() {
    if(!SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    auto window=SDL_CreateWindow("C++ GL parity",32,32,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window) return 2;
    auto context=SDL_GL_CreateContext(window);
    if(!context) return 3;
    gl::loadApi();
    for(int input=1;input<5;++input) for(int output=1;output<5;++output) {
        gl::GLId id{};
        gl::oglCreateTextureHandle(id);
        std::vector<uint8_t> data(6*input);
        for(size_t i=0;i<data.size();++i) data[i]=static_cast<uint8_t>(i*17+3);
        gl::oglUploadTextureData(id,3,2,input,output,false,data);
        std::vector<uint8_t> result(6*output);
        gl::oglReadTextureData(id,output,false,result);
        std::printf("%d:%d:",input,output);
        for(auto b:result) std::printf("%02x",b);
        std::puts("");
        for(auto filter:{gl::Filtering::Linear,gl::Filtering::Nearest}) for(bool mipmap:{false,true}) {
            gl::oglApplyTextureFiltering(id,filter,mipmap);
            int value;
            glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,&value);
            std::printf("%d\n",value);
        }
        for(auto wrap:{gl::Wrapping::Clamp,gl::Wrapping::Repeat,gl::Wrapping::Mirror}) {
            gl::oglApplyTextureWrapping(id,wrap);
            int value;
            glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,&value);
            std::printf("%d\n",value);
        }
        gl::oglDeleteTextureHandle(id);
        std::printf("%u:%u\n",id,glGetError());
    }
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
