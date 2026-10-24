#include "nijilive/opengl/backend.hpp"
#include "nijilive/opengl/blending.hpp"
#include <core/runtime_state.hpp>
#include <core/puppet.hpp>
#include <SDL3/SDL.h>
#include <iostream>
#include <stdexcept>
#include <string_view>
namespace gl = nijiexpose::ogl;
namespace core = nicxlive::core;
void check(bool ok,const char* message) { if (!ok) throw std::runtime_error(message); }
int main(int argc,char** argv) {
    const bool parity=argc>1 && std::string_view(argv[1])=="--d-parity";
    SDL_Window* window{};
    SDL_GLContext context{};
    int result=0;
    try {
        check(SDL_Init(SDL_INIT_VIDEO),SDL_GetError());
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
        window=SDL_CreateWindow("Native puppet render verification",64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
        check(window!=nullptr,SDL_GetError());
        context=SDL_GL_CreateContext(window);
        check(context!=nullptr,SDL_GetError());
        auto backend=std::make_shared<gl::Backend>();
        core::inSetRenderBackend(backend);
        backend->initializeRenderer();
        // Preserve the D initialization order, including attachment-before-bind.
        auto initError=glGetError();
        if(parity) std::cout<<"init-error:"<<initError<<'\n';
        check(initError==GL_NO_ERROR || initError==GL_INVALID_OPERATION,"Unexpected renderer initialization error");
        gl::inInitBlending();
        core::inSetViewport(64,64);
        core::inSetClearColor(0,0,0,0);
        std::vector<uint8_t> red(16*16*4);
        for(size_t i=0;i<red.size();i+=4) { red[i]=255; red[i+3]=255; }
        auto texture=std::make_shared<core::Texture>(red,16,16,4,4,false,false);
        check(texture->backendId()!=0,"Texture not allocated by native backend");
        auto readback=backend->readTexture(texture->backendId());
        check(readback==red,"Native texture data differs");
        GLint filter{};
        glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,&filter);
        check(filter==GL_LINEAR,"useMipmaps=false lost in native Texture constructor");
        auto part=core::nodes::Part::createSimple(texture,"Test quad");
        auto puppet=std::make_shared<core::Puppet>(part);
        check(core::getCurrentRenderBackend()==backend,"Puppet construction replaced the host resource backend");

        puppet->update();
        backend->beginScene();
        backend->drawPuppet(*puppet);
        check(dynamic_cast<core::render::QueueCommandEmitter*>(puppet->commandEmitter())!=nullptr,
              "Puppet must use the standard nicxlive queue emitter");
        std::vector<uint8_t> frame(64*64*4);
        backend->dumpViewport(frame,64,64);
        size_t opaque=0;
        for(size_t i=0;i<frame.size();i+=4) if(frame[i]==255 && frame[i+3]==255) ++opaque;
        if(!parity) std::cout<<"Rendered red pixels: "<<opaque<<'\n';
        check(opaque==256,"Native puppet did not render the 16x16 quad");
        auto quadError=glGetError();
        if(parity) std::cout<<"quad:"<<opaque<<':'<<quadError<<'\n';
        check(quadError==GL_NO_ERROR,"Puppet rendering produced GL error");
        // Locked updates must defer the upload until unlock, as in D.
        if(!parity) {
        texture->lock();
        auto green=red;
        for(size_t i=0;i<green.size();i+=4) { green[i]=0; green[i+1]=255; }
        texture->setData(green,4);
        check(backend->readTexture(texture->backendId())==red,"Locked texture uploaded early");
        texture->unlock();
        check(backend->readTexture(texture->backendId())==green,"Unlock did not upload modified texture");
        }
        core::nodes::PartDrawPacket packet;
        part->fillDrawPacket(*part,packet,false);
        auto maskPacket=packet;
        maskPacket.isMask=true;
        maskPacket.modelMatrix=gl::mat4::scale({.5f,1,1}) * packet.modelMatrix;
        core::RenderBackend::MaskApplyPacket apply;
        apply.kind=core::RenderBackend::MaskDrawableKind::Part;
        apply.partPacket=maskPacket;
        for(bool dodge : {false,true}) {
            backend->beginScene();
            backend->beginMask(!dodge);
            apply.isDodge=dodge;
            backend->applyMask(apply);
            backend->beginMaskContent();
            backend->drawPartPacket(packet);
            backend->endMask();
            backend->dumpViewport(frame,64,64);
            size_t greenCount=0;
            for(size_t i=0;i<frame.size();i+=4) if(frame[i+(parity?0:1)]==255 && frame[i+3]==255) ++greenCount;
            if(!parity) std::cout<<"Mask dodge="<<dodge<<" pixels="<<greenCount<<'\n';
            check(greenCount==128,"Mask/dodge coverage differs");
            auto maskError=glGetError();
            if(parity) std::cout<<"mask:"<<int(dodge)<<':'<<greenCount<<':'<<maskError<<'\n';
            // D part.d uses the main shader's offset location with partMaskShader.
            check(maskError==GL_NO_ERROR || maskError==GL_INVALID_OPERATION,"Unexpected mask GL error");
        }
        backend->beginScene();
        puppet->update();
        backend->drawPuppet(*puppet);
        backend->setDifferenceAggregationEnabled(true);
        backend->dumpViewport(frame,64,64);
        size_t beforeDifference=0;
        for(size_t i=0;i<frame.size();i+=4) if(frame[i+1]) ++beforeDifference;
        check(glGetError()==GL_NO_ERROR,"Error before difference collection");
        if(!parity) std::cout<<"Before difference pixels "<<beforeDifference<<'\n';
        check(backend->evaluateDifferenceAggregation(backend->mainAlbedoHandle(),64,64),"Difference collection failed");
        core::DifferenceEvaluationResult difference;
        check(backend->fetchDifferenceAggregationResult(difference),"Difference result missing");
        if(!parity) std::cout<<"Difference "<<difference.red<<','<<difference.green<<','<<difference.blue<<','<<difference.alpha<<'\n';
        check((parity?difference.red:difference.green)==256 && (parity?difference.green:difference.red)==0 && difference.blue==0,"GPU difference sum differs");
        if(parity) {
            std::cout<<"difference:"<<difference.red<<':'<<difference.green<<':'<<difference.blue<<':'<<difference.alpha<<':'<<glGetError()<<'\n';
            backend->endScene();
            std::cout<<"end-error:"<<glGetError()<<'\n';
            return 0;
        }
        check(!backend->fetchDifferenceAggregationResult(difference),"Difference result consumed twice");
        core::DynamicCompositePass composite;
        composite.surface=std::make_shared<core::DynamicCompositeSurface>();
        composite.surface->textureCount=3;
        for(auto& t:composite.surface->textures) t=std::make_shared<core::Texture>(16,16,4,false,false);
        composite.surface->stencil=std::make_shared<core::Texture>(16,16,4,true,false);
        composite.autoScaled=true;
        backend->beginScene();
        const auto cameraBeforeComposite = core::inGetCamera();
        backend->beginDynamicComposite(composite);
        const auto cameraInsideComposite = core::inGetCamera();
        check(cameraInsideComposite.scale.x == cameraBeforeComposite.scale.x &&
              cameraInsideComposite.scale.y == cameraBeforeComposite.scale.y &&
              cameraInsideComposite.position.x == cameraBeforeComposite.position.x &&
              cameraInsideComposite.position.y == cameraBeforeComposite.position.y,
              "nijimi composite must not overwrite the host camera");
        puppet->update();
        backend->drawPuppet(*puppet);
        auto offscreen=backend->readTexture(composite.surface->textures[0]->backendId());
        size_t offscreenGreen=0;
        for(size_t i=0;i<offscreen.size();i+=4) if(offscreen[i+1]==255 && offscreen[i+3]==255) ++offscreenGreen;
        backend->endDynamicComposite(composite);
        check(offscreenGreen==256,"Dynamic composite did not render full quad");
        int restoredWidth{},restoredHeight{};
        core::inGetViewport(restoredWidth,restoredHeight);
        check(restoredWidth==64 && restoredHeight==64,"Dynamic composite did not restore viewport");
        const auto isFramebuffer = reinterpret_cast<PFNGLISFRAMEBUFFERPROC>(SDL_GL_GetProcAddress("glIsFramebuffer"));
        check(isFramebuffer!=nullptr,"Framebuffer inspection unavailable");
        const auto oldFramebuffer = composite.surface->framebuffer;
        check(oldFramebuffer && isFramebuffer(oldFramebuffer),"Composite framebuffer not allocated");
        backend->destroyDynamicComposite(composite.surface);
        check(composite.surface->framebuffer==0,"Composite handle not released");
        check(!isFramebuffer(oldFramebuffer),"Destroyed composite framebuffer still exists");
        backend->beginDynamicComposite(composite);
        const auto recreatedFramebuffer = composite.surface->framebuffer;
        check(recreatedFramebuffer && isFramebuffer(recreatedFramebuffer),"Composite framebuffer not recreated");
        check(gl::glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Recreated composite framebuffer incomplete");
        backend->endDynamicComposite(composite);
        backend->destroyDynamicComposite(composite.surface);
        check(composite.surface->framebuffer==0,"Composite framebuffer not disposed");
        check(!isFramebuffer(recreatedFramebuffer),"Recreated composite framebuffer still exists after disposal");
        check(glGetError()==GL_NO_ERROR,"Mask, difference or composite GL error");
        if(!parity) {
            const bool khr=SDL_GL_ExtensionSupported("GL_KHR_blend_equation_advanced");
            const bool coherent=SDL_GL_ExtensionSupported("GL_KHR_blend_equation_advanced_coherent");
            std::cout<<"KHR available="<<khr<<" coherent="<<coherent<<'\n';
            for(bool forceFallback:{true,false}) {
                if(!khr && !forceFallback) continue;
                for(bool useCoherent:{false,true}) {
                    if(useCoherent && (!coherent || forceFallback))continue;
                    gl::nlSetBlendingCapabilitiesForTesting(khr,useCoherent,forceFallback);
                    check(gl::inUseMultistageBlending(gl::BlendMode::Multiply)==!forceFallback,"KHR route selection differs");
                    if(coherent) check((glIsEnabled(GL_BLEND_ADVANCED_COHERENT_KHR)!=0)==useCoherent,"Coherent GL state differs");
                    core::inSetClearColor(.25f,.5f,.75f,1);
                    backend->beginScene();
                    part->fillDrawPacket(*part,packet,false);
                    packet.blendMode=gl::BlendMode::Multiply;
                    // Deliberately contradict the core hint: the host owns route selection.
                    packet.useMultistageBlend=forceFallback;
                    backend->drawPartPacket(packet);
                    backend->dumpViewport(frame,64,64);
                    const auto center=(32*64+32)*4;
                    check(frame[center]==0 && std::abs(int(frame[center+1])-128)<=1 && frame[center+2]==0 && frame[center+3]==255,"KHR multiply framebuffer differs");
                    check(glGetError()==GL_NO_ERROR,"KHR blend/coherent/barrier GL error");
                    if(!forceFallback) {
                        using B=gl::BlendMode;
                        struct Case { B mode; std::array<float,3> rgb; };
                        const Case cases[]={
                            {B::Screen,{.25f,1,.75f}}, {B::Overlay,{0,1,.5f}},
                            {B::Darken,{0,.5f,0}}, {B::Lighten,{.25f,1,.75f}},
                            {B::ColorDodge,{.25f,1,.75f}}, {B::ColorBurn,{0,.5f,0}},
                            {B::HardLight,{0,1,0}}, {B::SoftLight,{.0625f,std::sqrt(.5f),.5625f}},
                            {B::Difference,{.25f,.5f,.75f}}, {B::Exclusion,{.25f,.5f,.75f}}
                        };
                        for(const auto& c:cases) {
                            backend->beginScene(); packet.blendMode=c.mode;
                            backend->drawPartPacket(packet); backend->dumpViewport(frame,64,64);
                            for(int channel=0;channel<3;++channel)
                                check(std::abs(int(frame[center+channel])-int(std::lround(c.rgb[channel]*255)))<=2,"KHR blend equation pixel mismatch");
                            check(frame[center+3]==255 && glGetError()==GL_NO_ERROR,"KHR blend equation GL error");
                        }
                        std::cout<<"All 11 KHR equations passed coherent="<<useCoherent<<'\n';
                        const auto originalTextures=part->textures;
                        part->textures[0]=std::make_shared<core::Texture>(backend->readTexture(originalTextures[0]->backendId()),16,16,4,4,false,false);
                        std::vector<uint8_t> emissionPixels(16*16*4),bumpPixels(16*16*4);
                        for(size_t i=0;i<emissionPixels.size();i+=4){emissionPixels[i]=64;emissionPixels[i+3]=255;bumpPixels[i+2]=192;bumpPixels[i+3]=255;}
                        part->textures[1]=std::make_shared<core::Texture>(emissionPixels,16,16,4,4,false,false);
                        part->textures[2]=std::make_shared<core::Texture>(bumpPixels,16,16,4,4,false,false);
                        backend->beginScene();
                        part->fillDrawPacket(*part,packet,false);
                        packet.blendMode=B::Screen;
                        backend->drawPartPacket(packet);
                        std::array<uint8_t,4> emission{},bump{};
                        GLint drawFramebuffer{},readFramebuffer{};
                        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&drawFramebuffer);
                        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&readFramebuffer);
                        gl::glBindFramebuffer(GL_READ_FRAMEBUFFER,drawFramebuffer);
                        gl::glReadBuffer(GL_COLOR_ATTACHMENT1);
                        glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,emission.data());
                        gl::glReadBuffer(GL_COLOR_ATTACHMENT2);
                        glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,bump.data());
                        gl::glReadBuffer(GL_COLOR_ATTACHMENT0);
                        gl::glBindFramebuffer(GL_READ_FRAMEBUFFER,readFramebuffer);
                        backend->dumpViewport(frame,64,64);
                        check(std::abs(int(frame[center])-64)<=1 && frame[center+1]==255 && std::abs(int(frame[center+2])-191)<=1,"Secondary KHR pass overwrote albedo");
                        std::cout<<"KHR auxiliary emission="<<int(emission[0])<<','<<int(emission[1])<<','<<int(emission[2])<<','<<int(emission[3])<<" bump="<<int(bump[0])<<','<<int(bump[1])<<','<<int(bump[2])<<','<<int(bump[3])<<'\n';
                        check(std::abs(int(emission[0])-64)<=1 && std::abs(int(bump[2])-192)<=1,"Secondary KHR output locations differ");
                        check(glGetError()==GL_NO_ERROR,"Secondary KHR pass GL error");
                        part->textures=originalTextures;
                    }
                    std::cout<<"KHR multiply pixels passed fallback="<<forceFallback<<" coherent="<<useCoherent<<'\n';
                }
            }
            gl::inInitBlending();
        }
        puppet.reset(); part.reset(); texture.reset();
        core::inDrainPendingTextureDisposals();
        std::cout<<"Native puppet rendering and texture lifecycle passed\n";
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n'; result=1;
    }
    if(context) SDL_GL_DestroyContext(context);
    if(window) SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
