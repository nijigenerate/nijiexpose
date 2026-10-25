// legacy D: nijiexpose/scene/package.d: SceneItem, in original operation order.
#include "item.hpp"
#include "render/renderer.hpp"
#include "nijilive/opengl/backend.hpp"
#include "nijilive/model_io.hpp"
#include <core/math/triangle.hpp>
#include <core/runtime_state.hpp>
#include <core/nodes/drawable.hpp>
#include <format>
namespace nijiexpose::scene {
void SceneItem::fitToViewport(float margin,bool enlarge) {
    if(!attachedParent.expired())throw std::runtime_error("Adjust the root model of an attached group");
    int width=0,height=0;nicxlive::core::inGetViewport(width,height);
    if(!std::isfinite(margin)||margin<0||2*margin>=std::min(width,height))throw std::runtime_error("Invalid margin");
    struct BoundsScope {bool previous=nicxlive::core::nodes::inGetUpdateBounds();BoundsScope(){nicxlive::core::nodes::inSetUpdateBounds(true);}~BoundsScope(){nicxlive::core::nodes::inSetUpdateBounds(previous);}} scope;
    auto b=getBounds();auto& camera=nicxlive::core::inGetCamera();auto& t=puppet->transform;
    double bw=std::abs(b[2]-b[0]),bh=std::abs(b[3]-b[1]);
    if(!(bw>0&&bh>0&&t.scale.x>0&&camera.scale.x>0&&camera.scale.y>0))throw std::runtime_error("Invalid model bounds or camera");
    double factor=std::min((width-2*margin)/(bw*camera.scale.x),(height-2*margin)/(bh*camera.scale.y));
    if(!enlarge)factor=std::min(factor,1.0);
    float scale=std::clamp(static_cast<float>(t.scale.x*factor),.25f,5.f);
    double ratio=scale/t.scale.x;
    float x=static_cast<float>(camera.position.x/camera.scale.x-((b[0]+b[2])*.5-t.translation.x)*ratio);
    float y=static_cast<float>(camera.position.y/camera.scale.y-((b[1]+b[3])*.5-t.translation.y)*ratio);
    t.scale={scale,scale};t.translation.x=x;t.translation.y=y;t.update();updateTransform();
}
namespace {
constexpr auto bindingKey="com.inochi2d.inochi-session.bindings";
constexpr auto animationKey="com.inochi2d.inochi-session.animations";
void store(Puppet& puppet,const char* key,const json::Array& array) {
    auto text=json::serialize(array);puppet.extData[key]={text.begin(),text.end()};
}
json::Value load(const std::vector<uint8_t>& data) {return json::parse({reinterpret_cast<const char*>(data.data()),data.size()});}
float minZ(const std::shared_ptr<Node>& node) {
    float result=node->zSort();for(auto& child:node->children)result=std::min(result,minZ(child));return result;
}
bool surroundingTriangle(Vec2 pt,nicxlive::core::nodes::MeshData& mesh) {
    auto sign=[](Vec2 p1,Vec2 p2,Vec2 p3){return (p1.x-p3.x)*(p2.y-p3.y)-(p2.x-p3.x)*(p1.y-p3.y);};
    for(size_t i=0;i<mesh.indices.size();i+=3){
        auto p1=mesh.vertices.at(mesh.indices.at(i)),p2=mesh.vertices.at(mesh.indices.at(i+1)),p3=mesh.vertices.at(mesh.indices.at(i+2));
        auto d1=sign(pt,p1,p2),d2=sign(pt,p2,p3),d3=sign(pt,p3,p1);
        if(!((d1<0 || d2<0 || d3<0) && (d1>0 || d2>0 || d3>0)))return true;
    }return false;
}
}
std::string SceneItem::name() const {auto pos=filePath.find_last_of("/\\");return pos==std::string::npos?filePath:filePath.substr(pos+1);}
void SceneItem::saveBindings() {json::Array a;for(auto& b:bindings)a.push_back(b->serialize());store(*puppet,bindingKey,a);model_io::inWriteINPExtensions(puppet,filePath);}
void SceneItem::saveAnimations() {json::Array a;for(auto& b:animations)a.push_back(b->serialize());store(*puppet,animationKey,a);model_io::inWriteINPExtensions(puppet,filePath);}
bool SceneItem::tryLoadBindings() {
    auto it=puppet->extData.find(bindingKey);if(it==puppet->extData.end())return false;
    std::vector<std::shared_ptr<tracking::TrackingBinding>> pre;
    auto data=load(it->second);if(!data.is_null())for(auto& value:data.as_array()){auto b=std::make_shared<tracking::TrackingBinding>();b->deserializeFromJson(value);pre.push_back(b);}
    bindings.clear();for(auto& b:pre)if(b->finalize(puppet))bindings.push_back(b);return true;
}
bool SceneItem::tryLoadAnimations() {
    auto it=puppet->extData.find(animationKey);if(it==puppet->extData.end())return false;
    std::vector<std::shared_ptr<animation::AnimationControl>> pre;
    auto data=load(it->second);if(!data.is_null())for(auto& value:data.as_array()){auto b=std::make_shared<animation::AnimationControl>();b->deserializeFromJson(value);pre.push_back(b);}
    animations.clear();for(auto& b:pre)if(b->finalize(*player))animations.push_back(b);return true;
}
void SceneItem::genBindings() {
    std::vector<std::pair<std::shared_ptr<tracking::Parameter>,int>> links;
    for(auto& b:bindings)links.emplace_back(b->param,b->axis);
    for(auto& param:puppet->parameters) {
        bool driven=false;for(auto& driver:puppet->getDrivers())if(driver->affectsParameter(param)){driven=true;break;}if(driven)continue;
        for(int i=0;i<(param->isVec2?2:1);++i){
            if(std::find(links.begin(),links.end(),std::make_pair(param,i))!=links.end())continue;
            auto b=std::make_shared<tracking::TrackingBinding>();b->param=param;b->axis=i;b->type(tracking::BindingType::RatioBinding);
            std::static_pointer_cast<tracking::RatioTrackingBinding>(b->delegated)->inRange={0,1};b->outRangeToDefault();
            b->name=param->isVec2?param->name+(i==0?" (X)":" (Y)"):param->name;bindings.push_back(b);
        }
    }
}
void SceneItem::genAnimationControls() {
    std::map<std::string,std::shared_ptr<animation::AnimationControl>> acs;for(auto& ac:animations)acs[ac->name]=ac;
    for(auto& [name,anim]:puppet->animations)if(!acs.contains(name)){auto ac=std::make_shared<animation::AnimationControl>();ac->name=name;ac->finalize(*player);animations.push_back(ac);}
}
void SceneItem::sleep(){for(auto& ac:animations)ac->sleep();}
void SceneItem::awake(){for(auto& ac:animations)ac->awake();}
std::array<float,4> SceneItem::getBounds() {
    auto root=shared_from_this();while(auto parent=root->attachedParent.lock())root=parent;
    if(!puppetRoot)return {NAN,NAN,NAN,NAN};
    auto matrix=root->puppet->transform.matrix();auto b=puppetRoot->getCombinedBounds(true);
    auto tl=matrix.transformPoint({b[0],b[1],0}),br=matrix.transformPoint({b[2],b[3],0});return {tl.x,tl.y,br.x,br.y};
}
bool SceneItem::attachTo(const std::shared_ptr<SceneItem>& parent) {
    if(!parent)return false;
    auto moving=parent;
    for(;;){if(moving.get()==this)return false;auto next=moving->attachedParent.lock();if(!next)break;moving=next;}
    auto rel=moving->puppet->transform.matrix().inverse().transformPoint(puppet->transform.translation);
    Vec2 scale{puppet->transform.scale.x/moving->puppet->transform.scale.x,puppet->transform.scale.y/moving->puppet->transform.scale.y};
    auto& drawables=parent->puppet->getRootParts();std::sort(drawables.begin(),drawables.end(),[](auto& a,auto& b){return a->zSort()<b->zSort();});
    for(auto node:drawables){
        auto d=std::dynamic_pointer_cast<nicxlive::core::nodes::Drawable>(node);if(!d)continue;
        auto transform=d->transform();auto pos=transform.matrix().inverse().transformPoint({rel.x,rel.y,0});
        Vec2 target{scale.x/transform.scale.x,scale.y/transform.scale.y};
        if(!surroundingTriangle({pos.x,pos.y},d->getMesh()))continue;
        auto previous=attachedParent.lock();
        if(previous)std::erase(previous->children,shared_from_this());
        puppetRoot->reparent(node,0);node->transformChanged();puppetRoot->localTransform.translation=pos;puppetRoot->localTransform.scale=target;
        node->transformChanged();puppetRoot->setAbsZSort(minZ(moving->puppet->root)-1);puppetRoot->setPinToMesh(true);
        if(previous&&previous!=parent)previous->puppet->rescanNodes();
        parent->puppet->rescanNodes();attachedParent=parent;parent->children.push_back(shared_from_this());return true;
    }return false;
}
bool SceneItem::detach() {
    auto keepAlive=shared_from_this();
    auto parent=attachedParent.lock();if(!parent)return false;
    auto root=parent;while(auto next=root->attachedParent.lock())root=next;
    auto cur=puppetRoot->transform();cur.translation=root->puppet->transform.matrix().transformPoint(cur.translation);
    cur.scale.x*=root->puppet->transform.scale.x;cur.scale.y*=root->puppet->transform.scale.y;cur.rotation.z+=root->puppet->transform.rotation.z;
    std::erase(parent->children,shared_from_this());
    puppetRoot->reparent(nullptr,0,true);
    puppet->setRootNode(puppetRoot);puppet->transform=cur;puppet->transform.update();
    puppetRoot->localTransform.translation={0,0,0};puppetRoot->localTransform.scale={1,1};puppetRoot->localTransform.rotation={0,0,0};
    puppetRoot->transformChanged();puppetRoot->zSort(0);puppet->rescanNodes();attachedParent.reset();puppetRoot->setPinToMesh(false);parent->puppet->rescanNodes();
    auto traverse=[](auto&& self,auto& item)->void{item->updateTransform();for(auto& child:item->children)self(self,child);};
    for(auto& child:children)traverse(traverse,child);return true;
}
bool SceneItem::intersects(Vec2 mouse) {auto b=getBounds();return !(mouse.x>=b[0]+std::abs(b[2]-b[0]) || mouse.x<=b[0] || mouse.y>=b[1]+std::abs(b[3]-b[1]) || mouse.y<=b[1]);}
void SceneItem::draw(){for(auto& b:bindings)b->update();for(auto& ac:animations)ac->update();player->update(static_cast<float>(nicxlive::core::deltaTime()));puppet->update();if(fitOnFirstDraw){fitOnFirstDraw=false;try{fitToViewport(80,false);}catch(const std::exception&){/* Empty/nonfinite models have no fit rectangle. */}}if(attachedParent.expired()){auto backend=std::dynamic_pointer_cast<ApplicationRenderBackend>(nicxlive::core::getCurrentRenderBackend());if(backend)backend->drawPuppet(*puppet);}for(auto& b:bindings)b->lateUpdate();}
void SceneItem::updateTransform(){auto b=getBounds();startPos={puppet->transform.translation.x,puppet->transform.translation.y};targetScale=puppet->transform.scale.x;targetPos=startPos;targetSize={b[2]-b[0],b[3]-b[1]};}
std::string SceneItem::dump(int index) const {std::string result=std::format("{}{}({:.2f})\n",std::string(index*2,' '),name(),zSort);for(auto& child:children)result+=child->dump(index+1);return result;}
}
