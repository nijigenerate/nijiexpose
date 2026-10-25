#include "scene/package.hpp"
#include "tracking/context.hpp"
#include "nijilive/model_io.hpp"
#include "nijilive/opengl/backend.hpp"
#include <core/runtime_state.hpp>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <iostream>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){
    using namespace nijiexpose;namespace core=nicxlive::core;
    try {
        check(argc==2,"Expected isolated model file");
        check(SDL_Init(SDL_INIT_VIDEO),SDL_GetError());SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
        auto window=SDL_CreateWindow("SceneItem verification",32,32,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);check(window,SDL_GetError());check(SDL_GL_CreateContext(window),SDL_GetError());
        auto backend=std::make_shared<ogl::Backend>();core::inSetRenderBackend(backend);backend->initializeRenderer();core::inSetViewport(32,32);
        ImGui::CreateContext();tracking::insInitExpressions();
        auto make=[&]{auto item=std::make_shared<scene::SceneItem>();item->filePath=argv[1];item->puppet=model_io::inLoadPuppet(argv[1]);item->puppet->root->build();item->puppetRoot=item->puppet->root;item->player=std::make_unique<animation::AnimationPlayer>(item->puppet);return item;};
        auto item=make();check(!item->tryLoadBindings()&&!item->tryLoadAnimations(),"Absent extensions should not replace state");
        item->genBindings();item->genBindings();check(item->bindings.size()==1,"Generated duplicate parameter binding");
        item->genAnimationControls();item->genAnimationControls();check(item->animations.size()==1,"Generated duplicate animation control");
        auto ratio=std::dynamic_pointer_cast<tracking::RatioTrackingBinding>(item->bindings[0]->delegated);ratio->sourceName="EyeBlinkLeft";ratio->inverse=true;ratio->dampenLevel=7;
        item->animations[0]->type=animation::TriggerType::Event;item->animations[0]->playEvent=1;
        item->saveBindings();item->saveAnimations();auto reload=make();check(reload->tryLoadBindings()&&reload->tryLoadAnimations(),"Saved extensions not loaded");
        check(reload->bindings.size()==1&&reload->animations.size()==1,"Loaded controls missing");auto restored=std::dynamic_pointer_cast<tracking::RatioTrackingBinding>(reload->bindings[0]->delegated);
        check(restored->inverse&&restored->dampenLevel==7&&restored->sourceName=="EyeBlinkLeft","Binding configuration lost");check(reload->animations[0]->type==animation::TriggerType::Event&&reload->animations[0]->playEvent==1,"Animation events lost");
        core::nodes::inSetUpdateBounds(true);item->puppet->update();auto bounds=item->getBounds();check(item->intersects({(bounds[0]+bounds[2])/2,(bounds[1]+bounds[3])/2}),"Bounds center rejected");check(!item->intersects({bounds[0],bounds[1]}),"D excludes rectangle edges");
        auto child=make();child->puppet->update();check(child->attachTo(item),"Mesh attachment failed");check(child->attachedParent.lock()==item&&item->children.size()==1&&child->puppetRoot->pinToMesh,"Attachment state incomplete");
        auto meshParent=child->puppetRoot->parentPtr();
        check(child->detach(),"Detach failed");check(child->attachedParent.expired()&&item->children.empty()&&!child->puppetRoot->pinToMesh,"Detach state incomplete");
        check(std::find(meshParent->children.begin(),meshParent->children.end(),child->puppetRoot)==meshParent->children.end(),"Detached root remained owned by old mesh");
        check(!child->detach(),"Unattached detach must return false");
        check(!item->attachTo(item),"Self attachment accepted");
        check(child->attachTo(item),"Second attachment failed");
        check(!item->attachTo(child),"Cyclic attachment accepted");
        auto distant=make();distant->puppet->transform.translation.x=100000;distant->puppet->transform.update();distant->puppet->update();
        check(!child->attachTo(distant),"Unexpected distant attachment");
        check(child->attachedParent.lock()==item&&item->children.size()==1,"Failed reattach damaged ownership");
        check(child->detach(),"Second detach failed");
        {
            auto parent=make(),attached=make();parent->puppet->update();attached->puppet->update();
            check(attached->attachTo(parent),"Lifetime attachment failed");
            std::weak_ptr<scene::SceneItem> wp=parent,wc=attached;
            parent.reset();attached.reset();
            check(wp.expired()&&wc.expired(),"SceneItem ownership cycle retained objects");
        }
        {
            scene::Scene testScene;
            auto parent=make(),attached=make();parent->puppet->update();attached->puppet->update();
            check(attached->attachTo(parent),"Removal attachment failed");
            testScene.sceneItems={parent,attached};
            check(testScene.removeSceneItem(parent),"Parent removal failed");
            check(attached->attachedParent.expired()&&!attached->puppetRoot->pinToMesh,"Parent removal did not detach child");
            check(testScene.sceneItems.size()==1,"Parent removal removed child from scene");
            std::weak_ptr<scene::SceneItem> wp=parent;parent.reset();check(wp.expired(),"Removed parent retained");
            parent=make();parent->puppet->update();attached->puppet->update();
            check(attached->attachTo(parent),"Child removal attachment failed");
            testScene.sceneItems.push_back(parent);
            check(testScene.removeSceneItem(attached)&&parent->children.empty(),"Child removal left owning parent link");
            std::weak_ptr<scene::SceneItem> wc=attached;
            std::weak_ptr<core::Puppet> puppet=attached->puppet;
            std::weak_ptr<core::nodes::Node> root=attached->puppetRoot;
            attached.reset();
            check(wc.expired()&&puppet.expired()&&root.expired(),"Removed child model remained owned");
        }
        {
            auto isFramebuffer=reinterpret_cast<PFNGLISFRAMEBUFFERPROC>(SDL_GL_GetProcAddress("glIsFramebuffer"));
            check(isFramebuffer!=nullptr,"glIsFramebuffer unavailable");
            auto texture=std::make_shared<core::Texture>(16,16);
            auto makeSurface=[&]{auto s=std::make_shared<core::DynamicCompositeSurface>();s->textures[0]=texture;s->textureCount=1;return s;};
            auto first=makeSurface(),second=makeSurface();
            auto render=[&](const auto& surface){core::DynamicCompositePass pass;pass.surface=surface;backend->beginDynamicComposite(pass);backend->endDynamicComposite(pass);};
            render(first);render(second);
            auto firstId=first->framebuffer,secondId=second->framebuffer;
            check(firstId!=secondId&&isFramebuffer(firstId)&&isFramebuffer(secondId),"Shared textures must not share FBO ownership");
            backend->destroyDynamicComposite(first);
            check(first->framebuffer==0&&!isFramebuffer(firstId)&&isFramebuffer(secondId),"Surface destruction invalidated wrong FBO");
            backend->destroyDynamicComposite(first); // Idempotent.
            second.reset();ogl::oglCollectDynamicFramebuffers();
            check(!isFramebuffer(secondId),"Expired surface retained FBO while texture remained alive");
            render(first);auto recreated=first->framebuffer;
            check(isFramebuffer(recreated),"FBO recreation failed");
            ogl::oglCollectDynamicFramebuffers(true);
            check(first->framebuffer==0&&!isFramebuffer(recreated),"Renderer cleanup retained FBO");
        }
        item->sleep();item->awake();check(item->animations[0]->eventQueue.size()>=2,"Sleep/wake events missing");
        item->updateTransform();check(item->targetScale==item->puppet->transform.scale.x,"Target scale not initialized");
        auto scene=scene::insScene();scene->space=std::make_shared<tracking::VirtualSpace>();scene->shouldPostProcess=false;
        scene->addPuppet(argv[1],model_io::inLoadPuppet(argv[1]));check(scene->sceneItems.size()==1,"Scene add failed");
        double now=0;core::inSetTimingFunc([&]{return now;});scene::neSetMeasureFPS(true);
        for(int i=0;i<7;++i){now+=1;scene->update();}
        check(scene->sleeping&&scene->zoneInactiveTimer==6,"Scene inactivity transition failed");check(scene::neGetFPS()==1,"FPS accumulation failed");
        tracking::insCleanupExpressions();ImGui::DestroyContext();std::cout<<"SceneItem extension persistence, control generation, bounds, mesh attachment and detach passed\n";return 0;
    }catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}
}
