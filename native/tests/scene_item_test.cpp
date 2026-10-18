#include "nijiexpose/scene/package.hpp"
#include "nijiexpose/tracking/context.hpp"
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
        auto child=make();child->puppet->update();check(child->attachTo(item),"Mesh attachment failed");check(child->attachedParent==item&&item->children.size()==1&&child->puppetRoot->pinToMesh,"Attachment state incomplete");
        check(child->detach(),"Detach failed");check(!child->attachedParent&&item->children.empty()&&!child->puppetRoot->pinToMesh,"Detach state incomplete");
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
