#include "nijilive/model_io.hpp"
#include "render/vulkan/backend.hpp"
#include "render/settings.hpp"
#include <core/runtime_state.hpp>
#include <core/timing.hpp>
#include <core/nodes/part.hpp>
#include <imgui_impl_sdl3.h>
#include <iostream>
#include <functional>
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
    backend->addBasicLightingPostProcess();nijiexpose::render::inSceneAmbientLight={.25f,1,1};
    pixels=frame([&]{backend->drawPartPacket(packet);backend->postProcessScene();});
    for(size_t i=0;i<pixels.size();i+=4)if(pixels[i+3])check(abs(int(pixels[i])-64)<=1 && pixels[i+3]==255,"Ambient light/alpha differs");
    pixels=frame([&]{backend->drawPartPacket(packet);});
    for(size_t i=0;i<pixels.size();i+=4)if(pixels[i+3])check(pixels[i]==255,"Disabled postprocessing affected color");
    auto maskData=red;for(size_t i=0;i<maskData.size();i+=4) maskData[i+3]=64;
    auto maskTexture=std::make_shared<core::Texture>(maskData,16,16,4,4,false,false);
    core::RenderBackend::MaskApplyPacket mask;mask.kind=core::RenderBackend::MaskDrawableKind::Part;mask.partPacket=packet;
    mask.partPacket.node.reset();mask.partPacket.textureBackendIds[0]=maskTexture->backendId();mask.partPacket.isMask=true;
    core::DynamicCompositePass composite;composite.surface=std::make_shared<core::DynamicCompositeSurface>();composite.surface->textureCount=3;
    for(auto& t:composite.surface->textures)t=std::make_shared<core::Texture>(64,64,4,false,false);
    for(bool offscreen:{false,true})for(bool dodge:{false,true})for(float threshold:{.1f,.5f}) {
     mask.isDodge=dodge;mask.partPacket.maskThreshold=threshold;
     pixels=frame([&]{
      if(offscreen)backend->beginDynamicComposite(composite);
      backend->beginMask(!dodge);backend->applyMask(mask);backend->beginMaskContent();backend->drawPartPacket(packet);backend->endMask();
      if(offscreen)backend->endDynamicComposite(composite);
     },offscreen?composite.surface->textures[0]->backendId():0);
     bool drawn=false;for(size_t i=3;i<pixels.size();i+=4)drawn|=pixels[i]!=0;
     check(drawn==((threshold<.25f)!=dodge),"Mask threshold/dodge/composite coverage differs");
    }
    backend->destroyDynamicComposite(composite.surface);
   }
   core::inSetRenderBackend(nullptr);backend->shutdown();backend.reset();
   ImGui_ImplSDL3_Shutdown();
  }
  ImGui::DestroyContext();SDL_DestroyWindow(window);SDL_Quit();
  std::cout<<"Vulkan opacity, lighting toggle, empty slot and root/composite masks passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
