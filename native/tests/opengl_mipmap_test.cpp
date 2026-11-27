#include "nijilive/opengl/backend.hpp"
#include "nijilive/opengl/blending.hpp"
#include <core/runtime_state.hpp>
#include <SDL3/SDL.h>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace gl = nijiexpose::ogl;
namespace core = nicxlive::core;
namespace {
void check(bool ok,const char* message) { if (!ok) throw std::runtime_error(message); }

void verifyMipmaps(gl::Backend& backend) {
    constexpr int size=256;
    std::vector<uint8_t> stripes(size*size*4);
    auto writeStripes=[&](uint8_t intensity) {
        for(int y=0;y<size;++y) for(int x=0;x<size;++x) {
            const size_t offset=(y*size+x)*4;
            stripes[offset]=(x%2)*intensity;
            stripes[offset+3]=255;
        }
    };
    auto readLastMip=[](const core::Texture& texture,int level) {
        std::array<uint8_t,4> pixel{};
        gl::oglBindTextureHandle(texture.backendId(),0);
        gl::glGetTexImage(GL_TEXTURE_2D,level,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
        return pixel;
    };
    writeStripes(255);
    auto texture=std::make_shared<core::Texture>(stripes,size,size,4,4,false,true);
    gl::enableTextureMipmaps(*texture);
    check(readLastMip(*texture,8)==std::array<uint8_t,4>{128,0,0,255},"Upload did not generate averaged mip levels");
    texture->setAnisotropy(1.f);

    auto part=core::nodes::Part::createSimple(texture,"Mipmapped stripes");
    auto puppet=std::make_shared<core::Puppet>(part);
    part->setPuppet(puppet);
    puppet->transform.scale={.2f,.2f};
    puppet->transform.update();
    puppet->update();
    backend.beginScene();
    backend.drawPuppet(*puppet);
    std::vector<uint8_t> image(64*64*4);
    backend.dumpViewport(image,64,64);
    check(image[(32*64)*4+3]==0,"Stripe fixture was not scaled to fit the viewport");
    for(int x=16;x<48;++x) {
        const size_t offset=(32*64+x)*4;
        if(std::abs(int(image[offset])-128)>1 || image[offset+3]!=255)
            throw std::runtime_error("Minified GL texture at x="+std::to_string(x)+
                " has red="+std::to_string(image[offset])+" alpha="+std::to_string(image[offset+3]));
    }
    texture->setAnisotropy(8.f);
    const bool anisotropyAvailable=SDL_GL_ExtensionSupported("GL_ARB_texture_filter_anisotropic") ||
                                   SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic");
    if(anisotropyAvailable) {
        GLfloat anisotropy{};
        glGetTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY,&anisotropy);
        check(anisotropy>1.f && anisotropy<=8.f,"Linear filtering ignored the anisotropy setting");
    }
    texture->setFiltering(core::Filtering::Nearest);
    GLint filter{};
    glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,&filter);
    check(filter==GL_NEAREST,"Nearest filtering samples mip levels");
    if(anisotropyAvailable) {
        GLfloat anisotropy{};
        glGetTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY,&anisotropy);
        check(anisotropy==1.f,"Nearest filtering uses anisotropy");
    }
    writeStripes(128);
    texture->setData(stripes,4);
    check(readLastMip(*texture,8)==std::array<uint8_t,4>{64,0,0,255},"Texture update kept stale mip levels");
    texture->setFiltering(core::Filtering::Linear);
    glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,&filter);
    check(filter==GL_LINEAR_MIPMAP_LINEAR,"Linear filtering did not restore mip sampling");

    auto makeComposite=[] {
        core::DynamicCompositePass pass;
        pass.surface=std::make_shared<core::DynamicCompositeSurface>();
        pass.surface->textureCount=3;
        for(auto& tex:pass.surface->textures) tex=std::make_shared<core::Texture>(16,16);
        return pass;
    };
    auto parent=makeComposite();
    auto child=makeComposite();
    for(float level:{1.f,0.f}) {
        backend.beginScene();
        backend.beginDynamicComposite(parent);
        backend.beginDynamicComposite(child);
        check(gl::glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Nested mipmap framebuffer incomplete");
        for(unsigned plane=0;plane<3;++plane) {
            std::array<float,3> color{};
            color[plane]=level;
            gl::glDrawBuffer(GL_COLOR_ATTACHMENT0+plane);
            glClearColor(color[0],color[1],color[2],1.f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        backend.endDynamicComposite(child);
        GLint framebuffer{};
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&framebuffer);
        check(static_cast<GLuint>(framebuffer)==parent.surface->framebuffer,"Composite mip refresh did not restore the parent target");
        for(unsigned plane=0;plane<3;++plane) {
            std::array<uint8_t,4> expected{0,0,0,255};
            expected[plane]=static_cast<uint8_t>(level*255);
            check(readLastMip(*child.surface->textures[plane],4)==expected,
                  "Composite redraw did not refresh all color attachments");
        }
        backend.endDynamicComposite(parent);
    }
    backend.destroyDynamicComposite(child.surface);
    backend.destroyDynamicComposite(parent.surface);
    check(glGetError()==GL_NO_ERROR,"OpenGL mipmap processing produced a GL error");
    std::cout<<"OpenGL texture minification and nested composite mip refresh passed\n";
}
}

int main() {
    SDL_Window* window{};
    SDL_GLContext context{};
    int result=0;
    try {
        check(SDL_Init(SDL_INIT_VIDEO),SDL_GetError());
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
        window=SDL_CreateWindow("OpenGL mipmap regression",64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
        check(window!=nullptr,SDL_GetError());
        context=SDL_GL_CreateContext(window);
        check(context!=nullptr,SDL_GetError());
        auto backend=std::make_shared<gl::Backend>();
        core::inSetRenderBackend(backend);
        backend->initializeRenderer();
        const auto initError=glGetError();
        check(initError==GL_NO_ERROR || initError==GL_INVALID_OPERATION,"Unexpected GL initialization error");
        gl::inInitBlending();
        core::inSetViewport(64,64);
        core::inSetClearColor(0,0,0,0);
        verifyMipmaps(*backend);
        core::inSetRenderBackend(nullptr);
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        result=1;
    }
    if(context) SDL_GL_DestroyContext(context);
    if(window) SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
