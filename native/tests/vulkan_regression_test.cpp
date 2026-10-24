#include "nijilive/model_io.hpp"
#include "render/vulkan/backend.hpp"
#include "render/settings.hpp"
#include <core/runtime_state.hpp>
#include <core/timing.hpp>
#include <core/nodes/part.hpp>
#include <imgui_impl_sdl3.h>
#include <iostream>
#include <functional>
#include <algorithm>
namespace core=nicxlive::core;
namespace vk=nijiexpose::vulkan;
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() {
 try {
  check(SDL_Init(SDL_INIT_VIDEO),SDL_GetError());
  auto window=SDL_CreateWindow("Vulkan pixel regression",64,64,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
  check(window,SDL_GetError());
  ImGui::CreateContext(); ImGui::GetIO().IniFilename=nullptr;
  {
   vk::Context context(window); ImGui_ImplSDL3_InitForVulkan(window); context.initializeImGui();
   core::inInit([] { return double(SDL_GetTicks())/1000.; });
   auto backend=std::make_shared<vk::Backend>(); core::inSetRenderBackend(backend); backend->initializeRenderer();
   core::inSetViewport(64,64); core::inSetClearColor(0,0,0,0);
   {
    auto empty=std::make_shared<core::Texture>(std::vector<uint8_t>{},0,0,4,4,false,false);
    check(backend->readTexture(empty->backendId())==std::vector<uint8_t>(4,0),"Empty slot placeholder not transparent");
    std::vector<uint8_t> red(16*16*4);
    for(size_t i=0;i<red.size();i+=4){red[i]=255;red[i+3]=255;}
    auto texture=std::make_shared<core::Texture>(red,16,16,4,4,false,false);
    auto part=core::nodes::Part::createSimple(texture,"Regression quad");
    auto puppet=std::make_shared<core::Puppet>(part); puppet->update();
    core::nodes::PartDrawPacket packet; part->fillDrawPacket(*part,packet,false);
    packet.modelMatrix=core::nodes::Mat4::identity();
    packet.renderMatrix=core::nodes::Mat4::scale({.1f,.1f,1});
    auto frame=[&](const std::function<void()>& draw,uint32_t read=0) {
     SDL_PumpEvents();check(context.beginFrame(),"Acquire failed");
     ImGui_ImplVulkan_NewFrame();ImGui_ImplSDL3_NewFrame();ImGui::NewFrame();ImGui::Render();
     backend->beginScene();draw();backend->endScene();backend->render();
     context.present(ImGui::GetDrawData(),backend->sceneImage());context.idle();
     return backend->readTexture(read?read:backend->renderImageHandle());
    };
    packet.opacity=.5f;
    auto pixels=frame([&]{backend->drawPartPacket(packet);});
    bool visible=false;
    for(size_t i=0;i<pixels.size();i+=4) if(pixels[i+3]){visible=true;check(abs(int(pixels[i])-128)<=1 && abs(int(pixels[i+3])-128)<=1,"Opacity not premultiplied");}
    check(visible,"Quad invisible");packet.opacity=1;
    if(context.advancedBlendSupported) {
     const bool coherent=context.advancedBlendCoherent;
     for(bool useCoherent : {coherent,false}) {
      context.advancedBlendCoherent=useCoherent;
      pixels=frame([&]{
       backend->drawPartPacket(packet);
       auto overlap=packet;overlap.clampedTint={.5f,1,1};
       overlap.blendMode=core::nodes::BlendMode::Multiply;backend->drawPartPacket(overlap);
       overlap.blendMode=core::nodes::BlendMode::Screen;backend->drawPartPacket(overlap);
      });
      for(size_t i=0;i<pixels.size();i+=4)if(pixels[i+3])check(abs(int(pixels[i])-192)<=2 && pixels[i+3]==255,"Advanced blend ordering differs");
     }
     context.advancedBlendCoherent=coherent;
     std::cout<<"Advanced overlap pixels passed, coherent feature="<<coherent<<", explicit barrier path tested\n";
    }
    backend->addBasicLightingPostProcess();nijiexpose::render::inSceneAmbientLight={.25f,1,1};
    pixels=frame([&]{backend->drawPartPacket(packet);backend->postProcessScene();});
    for(size_t i=0;i<pixels.size();i+=4)if(pixels[i+3])check(abs(int(pixels[i])-64)<=1 && pixels[i+3]==255,"Ambient light/alpha differs");
    pixels=frame([&]{backend->drawPartPacket(packet);});
    for(size_t i=0;i<pixels.size();i+=4)if(pixels[i+3])check(pixels[i]==255,"Disabled postprocessing affected color");
    auto dimData=red, emissionData=red;
    for(size_t i=0;i<red.size();i+=4) {
     dimData[i]=64;
     emissionData[i]=0;emissionData[i+2]=128;
    }
    auto dimTexture=std::make_shared<core::Texture>(dimData,16,16,4,4,false,false);
    auto emissionTexture=std::make_shared<core::Texture>(emissionData,16,16,4,4,false,false);
    auto luminous=packet;luminous.node.reset();
    luminous.textureBackendIds[0]=dimTexture->backendId();
    luminous.textureBackendIds[1]=emissionTexture->backendId();
    luminous.emissionStrength=1;
    auto requirePixel=[&](const std::vector<uint8_t>& image,int r,int b,int a,const char* message) {
     const size_t center=(32*64+32)*4;
     check(abs(int(image[center])-r)<=2 && image[center+1]<=2 &&
           abs(int(image[center+2])-b)<=2 && abs(int(image[center+3])-a)<=2,message);
    };
    for(float opacity:{1.f,.5f})for(bool post:{false,true})for(float light:{0.f,.25f,1.f}) {
     luminous.opacity=opacity;nijiexpose::render::inSceneAmbientLight={light,.75f,0.f};
     pixels=frame([&]{backend->drawPartPacket(luminous);if(post)backend->postProcessScene();});
     requirePixel(pixels,int(64*opacity*(post?light:1)),int(128*opacity),int(255*opacity),
                  "Emission was modulated by ambient light or opacity/alpha differs");
    }
    luminous.opacity=1;
    pixels=frame([&]{backend->drawPartPacket(luminous);backend->drawPartPacket(packet);backend->postProcessScene();});
    requirePixel(pixels,255,0,255,"Opaque foreground did not occlude emission");
    auto patternData=emissionData;
    for(size_t y=8;y<16;++y)for(size_t x=0;x<16;++x)patternData[(y*16+x)*4+2]=0;
    auto patternTexture=std::make_shared<core::Texture>(patternData,16,16,4,4,false,false);
    luminous.textureBackendIds[1]=patternTexture->backendId();
    nijiexpose::render::inSceneAmbientLight={0,0,0};
    pixels=frame([&]{backend->drawPartPacket(luminous);backend->postProcessScene();});
    auto emissionPixels=backend->readTexture(backend->mainEmissiveHandle());
    check(pixels==emissionPixels,"Emission composition flipped rows or changed alpha");
    luminous.textureBackendIds[1]=emissionTexture->backendId();
    pixels=frame([]{});
    check(std::all_of(pixels.begin(),pixels.end(),[](uint8_t v){return v==0;}),"Emission leaked into an empty frame");
    std::cout<<"Emission survives black/tinted ambient light, postprocessing toggle and opacity; occlusion passed\n";
    auto maskData=red;for(size_t i=0;i<maskData.size();i+=4) maskData[i+3]=64;
    auto maskTexture=std::make_shared<core::Texture>(maskData,16,16,4,4,false,false);
    core::RenderBackend::MaskApplyPacket mask;mask.kind=core::RenderBackend::MaskDrawableKind::Part;mask.partPacket=packet;
    mask.partPacket.node.reset();mask.partPacket.textureBackendIds[0]=maskTexture->backendId();mask.partPacket.isMask=true;
    core::DynamicCompositePass composite;composite.surface=std::make_shared<core::DynamicCompositeSurface>();composite.surface->textureCount=3;
    for(auto& t:composite.surface->textures)t=std::make_shared<core::Texture>(64,64,4,false,false);
    auto compositePacket=packet;compositePacket.node.reset();compositePacket.emissionStrength=1;
    for(int i=0;i<3;++i)compositePacket.textureBackendIds[i]=composite.surface->textures[i]->backendId();
    compositePacket.opacity=.5f;luminous.opacity=.5f;
    core::DynamicCompositePass outer;outer.surface=std::make_shared<core::DynamicCompositeSurface>();outer.surface->textureCount=3;
    for(auto& t:outer.surface->textures)t=std::make_shared<core::Texture>(64,64,4,false,false);
    auto outerPacket=compositePacket;
    for(int i=0;i<3;++i)outerPacket.textureBackendIds[i]=outer.surface->textures[i]->backendId();
    for(bool nested:{false,true}) {
     pixels=frame([&]{
      backend->beginDynamicComposite(composite);backend->drawPartPacket(luminous);backend->endDynamicComposite(composite);
      if(nested) {
       backend->beginDynamicComposite(outer);backend->drawPartPacket(compositePacket);backend->endDynamicComposite(outer);
       backend->drawPartPacket(outerPacket);
      }else backend->drawPartPacket(compositePacket);
      backend->postProcessScene();
     });
     requirePixel(pixels,0,nested?16:32,nested?32:64,"Composite lost emission or applied coverage twice");
    }
    compositePacket.clampedTint={1,1,.5f};
    pixels=frame([&]{
     backend->beginDynamicComposite(composite);backend->drawPartPacket(luminous);backend->endDynamicComposite(composite);
     backend->drawPartPacket(compositePacket);backend->postProcessScene();
    });
    requirePixel(pixels,0,16,64,"Composite tint did not affect its retained emission");
    backend->destroyDynamicComposite(outer.surface);luminous.opacity=1;
    for(bool offscreen:{false,true})for(bool dodge:{false,true})for(float threshold:{.1f,.5f}) {
     mask.isDodge=dodge;mask.partPacket.maskThreshold=threshold;
     pixels=frame([&]{
      if(offscreen)backend->beginDynamicComposite(composite);
      backend->beginMask(!dodge);backend->applyMask(mask);backend->beginMaskContent();backend->drawPartPacket(luminous);backend->endMask();
      if(offscreen)backend->endDynamicComposite(composite);
     },offscreen?composite.surface->textures[0]->backendId():0);
     bool drawn=false;for(size_t i=3;i<pixels.size();i+=4)drawn|=pixels[i]!=0;
     check(drawn==((threshold<.25f)!=dodge),"Mask threshold/dodge/composite coverage differs");
     emissionPixels=backend->readTexture(offscreen?composite.surface->textures[1]->backendId():backend->mainEmissiveHandle());
     bool emitted=false;for(size_t i=2;i<emissionPixels.size();i+=4)emitted|=emissionPixels[i]!=0;
     check(emitted==drawn,"Emission escaped root/composite stencil coverage");
    }
    std::cout<<"Emission orientation, empty-frame clearing, nested composite opacity and masks passed\n";
    backend->destroyDynamicComposite(composite.surface);
   }
   core::inSetRenderBackend(nullptr);backend->shutdown();backend.reset();
   ImGui_ImplSDL3_Shutdown();
  }
  ImGui::DestroyContext();SDL_DestroyWindow(window);SDL_Quit();
  std::cout<<"Vulkan opacity, lighting toggle, empty slot and root/composite masks passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
