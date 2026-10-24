// dlang_archive/source/nijiexpose/scene/package.d: Scene orchestration and pointer interaction.
#include "package.hpp"
#include "panels/tracking_state.hpp"
#include "plugins/package.hpp"
#include "render/spritebatch.hpp"
#include "log.hpp"
#include "nijilive/texture_utils.hpp"
#include "nijilive/opengl/runtime.hpp"
#include "nijiui/core/settings.hpp"
#include "nijiui/core/core.hpp"
#include "nijiui/input.hpp"
#include <core/runtime_state.hpp>
namespace nijiexpose::scene {
namespace core=nicxlive::core;
namespace {
thread_local bool measureFPS=false;thread_local double lastFPS=0,lastMeasureTime=0;thread_local uint32_t loopCount=0;
struct ItemHitTest {std::shared_ptr<SceneItem> item;ptrdiff_t index=-1;explicit operator bool()const{return bool(item);}};
thread_local ptrdiff_t selectedPuppet=-1;thread_local ItemHitTest draggingItem;thread_local bool hasDonePuppetSelect=false,isDragDown=false;thread_local std::shared_ptr<SceneItem> movingTarget;
thread_local core::math::Camera inCamera;
thread_local std::shared_ptr<Scene> singleton;
float damp(float pos,float target,double delta){return static_cast<float>((pos-target)*std::pow(0.001,delta)+target);}
int insertAfterAux(std::vector<std::shared_ptr<SceneItem>>& items,const std::shared_ptr<SceneItem>& previous,const std::shared_ptr<SceneItem>& target,int& base,bool rootOnly=false){
    int result=-1;for(auto item:items){if(!item->attachedParent.expired()&&rootOnly)continue;if(item==target)continue;else{item->zSort=static_cast<float>(base++);result=std::max(insertAfterAux(item->children,previous,target,base),result);}if(item==previous){target->zSort=static_cast<float>(base);result=base++;insertAfterAux(target->children,previous,target,base);}}return result;
}
int insertAfter(std::vector<std::shared_ptr<SceneItem>>& items,const std::shared_ptr<SceneItem>& previous,const std::shared_ptr<SceneItem>& target){int base=0;auto result=insertAfterAux(items,previous,target,base,true);std::sort(items.begin(),items.end(),[](auto a,auto b){return a->zSort<b->zSort;});return result;}
}
std::shared_ptr<Scene> insScene(){if(!singleton)singleton=std::make_shared<Scene>();return singleton;}
bool neGetMeasureFPS(){return measureFPS;}
void neSetMeasureFPS(bool value){if(value&&!measureFPS){lastMeasureTime=core::currentTime();loopCount=0;}measureFPS=value;}
double neGetFPS(){return lastFPS;}
void Scene::addPuppet(const std::string& path,const std::shared_ptr<Puppet>& puppet){auto item=std::make_shared<SceneItem>();item->filePath=path;item->fitOnFirstDraw=true;item->puppet=puppet;item->puppet->root->build();item->puppetRoot=puppet->root;item->attachedParent.reset();item->player=std::make_unique<animation::AnimationPlayer>(puppet);if(!item->tryLoadBindings())item->bindings.clear();if(!item->tryLoadAnimations())item->animations.clear();item->genBindings();item->genAnimationControls();if(sleeping)item->sleep();sceneItems.push_back(item);}
void Scene::init(){
    auto scene=insScene();scene->space=tracking::insLoadVSpace();AppBatch=std::make_shared<SpriteBatch>();scene->bgPath=nijiui::inSettingsGet<std::string>("bgPath");
    if(!scene->bgPath.empty())try{core::ShallowTexture tex(scene->bgPath);if(tex.channels==4)inTexPremultiply(tex.data);scene->backgroundImage=std::make_shared<core::Texture>(tex);}catch(const std::exception& ex){insLogErr("%s",ex.what());}
    scene->shouldPostProcess=nijiui::inSettingsGet<bool>("shouldPostProcess",true);auto ambient=nijiui::inSettingsGet<std::array<float,3>>("ambientLight",{1,1,1});ogl::inSceneAmbientLight={ambient[0],ambient[1],ambient[2]};auto color=nijiui::inSettingsGet<std::array<float,4>>("bgColor",{.5f,.5f,.5f,0});core::inSetClearColor(color[0],color[1],color[2],color[3]);
}
void Scene::cleanup(){
    tracking::insSaveVSpace(space);
    for(auto& source:space->getAllSources())if(source){if(source->isRunning())source->stop();source.reset();}
    while(!sceneItems.empty())removeSceneItem(sceneItems.back());
    draggingItem={};movingTarget.reset();selectedPuppet=-1;
    hasDonePuppetSelect=false;isDragDown=false;backgroundImage.reset();
}
void Scene::update(){
    int width,height;core::inGetViewport(width,height);core::inUpdate();space->update();core::getCurrentRenderBackend()->beginScene();
    if(backgroundImage){float tw=static_cast<float>(backgroundImage->width()),th=static_cast<float>(backgroundImage->height());float scale=std::max(width/tw,height/th);SpriteRect bounds{static_cast<float>(width/2),static_cast<float>(height/2),tw*scale,th*scale};AppBatch->draw(backgroundImage,bounds,{}, {bounds.width/2,bounds.height/2});AppBatch->flush();}
    for(auto& plugin:plugins::insPlugins){if(!plugin->isEnabled())continue;if(plugin->hasEvent("onUpdate"))plugin->callEvent("onUpdate",core::deltaTime());}
    if(!space->isCurrentZoneActive()){zoneInactiveTimer=static_cast<float>(zoneInactiveTimer+core::deltaTime());if(zoneInactiveTimer>=5&&!sleeping){for(auto& item:sceneItems)item->sleep();sleeping=true;}}
    else{zoneInactiveTimer=static_cast<float>(zoneInactiveTimer-core::deltaTime());if(sleeping){for(auto& item:sceneItems)item->awake();sleeping=false;}}
    zoneInactiveTimer=std::clamp(zoneInactiveTimer,0.f,6.f);for(auto& item:sceneItems)item->draw();core::getCurrentRenderBackend()->endScene();if(shouldPostProcess)core::getCurrentRenderBackend()->postProcessScene();
    if(measureFPS){double latest=core::currentTime();++loopCount;if(latest-lastMeasureTime>=1){lastFPS=loopCount/(latest-lastMeasureTime);lastMeasureTime=latest;loopCount=0;}}
}
std::shared_ptr<SceneItem> Scene::selectedSceneItem(){if(selectedPuppet<0||static_cast<size_t>(selectedPuppet)>=sceneItems.size())return nullptr;return sceneItems[selectedPuppet];}
bool Scene::removeSceneItem(const std::shared_ptr<SceneItem>& target) {
    auto item=target; // target may refer to an element erased below.
    if(std::find(sceneItems.begin(),sceneItems.end(),item)==sceneItems.end())return false;
    auto children=item->children;
    for(auto& child:children)child->detach();
    item->detach();
    std::erase(sceneItems,item);
    if(draggingItem.item==item)draggingItem={};
    if(movingTarget==item)movingTarget.reset();
    return true;
}
bool Scene::deleteSelectedSceneItem(){if(selectedPuppet<0||static_cast<size_t>(selectedPuppet)>=sceneItems.size())return false;removeSceneItem(sceneItems[static_cast<size_t>(selectedPuppet)]);selectedPuppet=-1;draggingItem={};movingTarget.reset();hasDonePuppetSelect=false;isDragDown=false;panels::insTrackingPanelRefresh();return true;}
void Scene::interact(){
    using namespace nijiui;using core::nodes::inSetUpdateBounds;constexpr auto left=MouseButton::Left;
    if(inInputMouseDownBegannijiui(left))return;int width,height;core::inGetViewport(width,height);inCamera=core::inGetCamera();auto mouse=inInputMousePosition();auto center=inCamera.getCenterOffset();mouse={(mouse.x-width/2+inCamera.position.x)/inCamera.scale.x,(mouse.y-height/2+inCamera.position.y)/inCamera.scale.y};
    auto hitTest=[&](const std::shared_ptr<Puppet>& stop){ItemHitTest result;auto& items=insScene()->sceneItems;for(size_t i=0;i<items.size();++i){auto item=items[i];if(item->puppet==stop)break;if(item->intersects(mouse)){result={item,static_cast<ptrdiff_t>(i)};item->updateTransform();}}return result;};
    if(!inInputWasMouseDown(left)&&inInputMouseDown(left)){
        if(!hasDonePuppetSelect){hasDonePuppetSelect=true;inSetUpdateBounds(true);draggingItem=hitTest(nullptr);if(draggingItem){selectedPuppet=draggingItem.index;panels::insTrackingPanelRefresh();movingTarget=draggingItem.item;while(auto parent=movingTarget->attachedParent.lock()){movingTarget=parent;movingTarget->updateTransform();}}else{selectedPuppet=-1;panels::insTrackingPanelRefresh();movingTarget.reset();}inSetUpdateBounds(false);}
    }else if(!inInputMouseDown(left)&&hasDonePuppetSelect)hasDonePuppetSelect=false;
    auto scaleItem=[&]{auto& item=*draggingItem.item;float delta=static_cast<float>((inInputMouseScrollDelta()*0.05)*(1-std::clamp(double(item.targetScale),0.0,0.45)));item.targetScale=std::clamp(item.targetScale+delta,.25f,5.f);};
    if(hasDonePuppetSelect&&draggingItem){ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);float prev=draggingItem.item->targetScale;scaleItem();if(prev!=draggingItem.item->targetScale){inSetUpdateBounds(true);auto item=draggingItem.item;auto b=item->puppet->root->getCombinedBounds(true,true);auto m=item->puppet->transform.matrix();float v[4]{};for(int r=0;r<4;++r)for(int c=0;c<4;++c)v[r]+=m[r][c]*b[c];item->targetSize={std::abs(v[2]-v[0]),std::abs(v[3]-v[1])};inSetUpdateBounds(false);}}
    if(inInputMouseDragging(left)&&hasDonePuppetSelect&&draggingItem){auto delta=inInputMouseDragDelta(left);auto item=draggingItem.item;item->targetPos={item->startPos.x+delta.x/inCamera.scale.x,item->startPos.y+delta.y/inCamera.scale.y};if(movingTarget)movingTarget->targetPos={movingTarget->startPos.x+delta.x/inCamera.scale.x,movingTarget->startPos.y+delta.y/inCamera.scale.y};}
    if(draggingItem){auto item=draggingItem.item;float cx=(center.x*2)+(item->targetSize.x/3),cy=static_cast<float>((center.y*2)+(item->targetSize.y/1.5));item->targetPos.x=std::clamp(item->targetPos.x,(inCamera.position.x-cx)*inCamera.scale.x,(inCamera.position.x+cx)*inCamera.scale.x);item->targetPos.y=std::clamp(item->targetPos.y,(inCamera.position.y-cy)*inCamera.scale.y,(inCamera.position.y+cy)*inCamera.scale.y);
        bool control=ImGui::IsKeyDown(ImGuiKey_LeftCtrl)||ImGui::IsKeyDown(ImGuiKey_RightCtrl);
        if(control){if(!item->attachedParent.expired()&&isDragDown){auto parent=item->attachedParent.lock();item->detach();insertAfter(insScene()->sceneItems,parent,item);movingTarget=item;item->updateTransform();}if(draggingItem&&isDragDown&&!inInputMouseDown(left)){auto hit=hitTest(item->puppet);if(hit.item&&item->attachTo(hit.item))selectedPuppet=insertAfter(insScene()->sceneItems,hit.item,item);}else if(draggingItem&&isDragDown){auto hit=hitTest(item->puppet);}}
        isDragDown=inInputMouseDown(left);if(control)scaleItem();if(movingTarget){auto& t=movingTarget->puppet->transform;double delta=inGetDeltaTime();t.translation={damp(t.translation.x,movingTarget->targetPos.x,delta),damp(t.translation.y,movingTarget->targetPos.y,delta),damp(t.translation.z,0,delta)};t.scale={damp(t.scale.x,movingTarget->targetScale,delta),damp(t.scale.y,movingTarget->targetScale,delta)};}
    }else isDragDown=false;
}
}
namespace nijiexpose::tracking {std::shared_ptr<VirtualSpace> insCurrentTrackingSpace(){return scene::insScene()->space;}}
