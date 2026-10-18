// Original package/ratio/expression/event/compound binding implementations.
#include "package.hpp"
#include "context.hpp"
#include "ext/exvmc.hpp"
#include "i18n/tr.hpp"
#include <core/timing.hpp>
#include <imgui.h>
#include <array>
#include <cmath>
#include <numbers>
namespace nijiexpose::tracking {
namespace {
constexpr std::array<const char*,5> bindingNames={"RatioBinding","ExpressionBinding","EventBinding","CompoundBinding","External"};
constexpr std::array<const char*,8> sourceNames={"Blendshape","BonePosX","BonePosY","BonePosZ","BoneRotRoll","BoneRotPitch","BoneRotYaw","KeyPress"};
constexpr std::array<const char*,3> methodNames={"WeightedSum","WeightedMul","Ordered"};
template<class E,size_t N> void readEnum(const json::Value& data,const char* key,E& out,const std::array<const char*,N>& names) {
    if(auto value=json::find(data,key);value && value->is_string())for(size_t i=0;i<N;++i)if(value->as_string()==names[i]){out=static_cast<E>(i);return;}
}
template<class T> void read(const json::Value& data,const char* key,T& out) {if(auto value=json::find(data,key))json::deserialize(*value,out);}
void readVec(const json::Value& data,const char* key,Vec2& out) {if(auto value=json::find(data,key);value && value->is_array()){auto& a=value->as_array();if(!a.empty())json::deserialize(a[0],out.x);if(a.size()>1)json::deserialize(a[1],out.y);}}
float& axis(Vec2& value,int index) {if(index==0)return value.x;if(index==1)return value.y;throw std::out_of_range("D parameter axis");}
float dampen(float pos,float target,double delta,double speed) {return static_cast<float>((pos-target)*std::pow(0.001,delta*speed)+target);}
float quantize(float v) {double scaled=double(v)/0.0001;return std::isinf(scaled)?v:static_cast<float>(std::nearbyint(scaled)*0.0001);}
float unmapAxis(TrackingBinding* binding,float value) {
    // Original Parameter.unmapValue computes (max-min)*offset+min.
    int a=binding->axis==0?0:1;
    return (axis(binding->param->max,a)-axis(binding->param->min,a))*value+axis(binding->param->min,a);
}
std::shared_ptr<ITrackingBinding> create(TrackingBinding* binding,BindingType type) {
    switch(type){case BindingType::RatioBinding:return std::make_shared<RatioTrackingBinding>(binding);case BindingType::ExpressionBinding:return std::make_shared<ExpressionTrackingBinding>(binding);case BindingType::EventBinding:return std::make_shared<EventTrackingBinding>(binding);case BindingType::CompoundBinding:return std::make_shared<CompoundTrackingBinding>(binding);default:return nullptr;}
}
std::string signature(TrackingBinding* binding) {return insExpressionGenerateSignature(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(binding)),binding->axis);}
}
void TrackingBinding::type(BindingType value) {type_=value;if(value!=BindingType::External)delegated=create(this,value);}
json::Value TrackingBinding::serialize() const {
    json::Object object{{"name",name},{"bindingType",bindingNames.at(static_cast<size_t>(type_))},{"param",param->uuid},{"axis",axis}};
    if(delegated)delegated->serializeSelf(object);return object;
}
void TrackingBinding::deserializeFromJson(const json::Value& data) {
    read(data,"name",name);readEnum(data,"bindingType",type_,bindingNames);type(type_);read(data,"param",paramUUID);read(data,"axis",axis);
    if(delegated)delegated->deserializeFromJson(data);
}
void TrackingBinding::update() {if(delegated){float updatedValue=std::numeric_limits<float>::quiet_NaN();if(delegated->update(updatedValue))tracking::axis(param->value,axis)=updatedValue;}}
void TrackingBinding::submit(const std::string& plugin,float value) {if(!pluginWeights.contains(plugin))pluginWeights[plugin]=1;sum+=value*pluginWeights.at(plugin);weights+=pluginWeights.at(plugin);}
void TrackingBinding::lateUpdate() {if(weights>0)tracking::axis(param->value,axis)+=std::round(sum/weights);}
void RatioTrackingBinding::serializeSelf(json::Object& o) const {
    o["sourceType"]=sourceNames.at(static_cast<size_t>(sourceType));o["sourceName"]=sourceName;o["sourceDisplayName"]=sourceDisplayName;o["inverse"]=inverse;
    o["inRange"]=json::Array{inRange.x,inRange.y};o["outRange"]=json::Array{outRange.x,outRange.y};o["dampenLevel"]=dampenLevel;
}
void RatioTrackingBinding::deserializeFromJson(const json::Value& data) {readEnum(data,"sourceType",sourceType,sourceNames);read(data,"sourceName",sourceName);read(data,"inverse",inverse);read(data,"dampenLevel",dampenLevel);readVec(data,"inRange",inRange);readVec(data,"outRange",outRange);createSourceDisplayName();}
void RatioTrackingBinding::outRangeToDefault() {outRange={axis(binding->param->min,binding->axis),axis(binding->param->max,binding->axis)};}
bool RatioTrackingBinding::update(float& result) {
    result=std::numeric_limits<float>::quiet_NaN();
    if(sourceName.empty()){axis(binding->param->value,binding->axis)=axis(binding->param->defaults,binding->axis);return false;}
    float src=0;auto space=insCurrentTrackingSpace();auto zone=space->currentZone();
    if(zone) switch(sourceType) {
        case SourceType::Blendshape:src=zone->getBlendshapeFor(sourceName);break;
        case SourceType::BonePosX:src=zone->getBoneFor(sourceName).position.x;break;
        case SourceType::BonePosY:src=zone->getBoneFor(sourceName).position.y;break;
        case SourceType::BonePosZ:src=zone->getBoneFor(sourceName).position.z;break;
        case SourceType::BoneRotRoll:src=static_cast<float>((180/std::numbers::pi)*zone->getBoneFor(sourceName).rotation.roll());break;
        case SourceType::BoneRotPitch:src=static_cast<float>((180/std::numbers::pi)*zone->getBoneFor(sourceName).rotation.pitch());break;
        case SourceType::BoneRotYaw:src=static_cast<float>((180/std::numbers::pi)*zone->getBoneFor(sourceName).rotation.yaw());break;
        default:throw std::logic_error("D source type assertion");
    }
    if(!space->hasAnyFocus()) {
        result=dampen(axis(binding->param->value,binding->axis),axis(binding->param->defaults,binding->axis),nicxlive::core::deltaTime(),1);
        result=quantize(axis(binding->param->value,binding->axis));return true;
    }
    float range=inRange.y-inRange.x,tmp=src-inRange.x,off=tmp/range,target=std::clamp(off,0.f,1.f);
    if(inverse)target=1.f-target;
    if(dampenLevel==0)inVal=target;else{inVal=dampen(inVal,target,nicxlive::core::deltaTime(),static_cast<float>(11-dampenLevel));inVal=quantize(inVal);}
    outVal=((outRange.y-outRange.x)*inVal)+outRange.x;result=outVal;return true;
}
void RatioTrackingBinding::createSourceDisplayName() {
    if(sourceType==SourceType::Blendshape){sourceDisplayName=sourceName;return;}
    static constexpr std::array<const char*,7> formats={"%s (X)","%s (Y)","%s (Z)","%s (Roll)","%s (Pitch)","%s (Yaw)","%s (Key)"};
    auto value=i18n::_(formats.at(static_cast<size_t>(sourceType)-1));auto index=value.find("%s");if(index!=std::string::npos)value.replace(index,2,sourceName);sourceDisplayName=value;
}
ExpressionTrackingBinding::ExpressionTrackingBinding(TrackingBinding* binding):binding_(binding),expr(std::make_unique<Expression>(signature(binding),"")) {}
void ExpressionTrackingBinding::serializeSelf(json::Object& o) const {o["expression"]=expr->expression();o["dampenLevel"]=dampenLevel;}
void ExpressionTrackingBinding::deserializeFromJson(const json::Value& data) {std::string source;read(data,"expression",source);read(data,"dampenLevel",dampenLevel);expr=std::make_unique<Expression>(signature(binding_),source);}
bool ExpressionTrackingBinding::update(float& result) {
    result=std::numeric_limits<float>::quiet_NaN();if(!expr)return false;
    float src=expr->call();if(!std::isfinite(src))return false;
    if(dampenLevel==0)outVal=src;else{outVal=dampen(outVal,src,nicxlive::core::deltaTime(),static_cast<float>(11-dampenLevel));outVal=quantize(outVal);}
    result=unmapAxis(binding_,outVal);return true;
}
void EventTrackingBinding::serializeSelf(json::Object& o) const {
    o["dampenLevel"]=dampenLevel;json::Array values;for(auto& item:valueMap)values.push_back(json::Object{{"type",sourceNames.at(static_cast<size_t>(item.type))},{"id",item.id},{"value",item.value}});o["value_map"]=std::move(values);
}
void EventTrackingBinding::deserializeFromJson(const json::Value& data) {
    valueMap.clear();read(data,"dampenLevel",dampenLevel);if(auto values=json::find(data,"value_map");values && values->is_array())for(auto& value:values->as_array()){EventMap item;readEnum(value,"type",item.type,sourceNames);read(value,"id",item.id);read(value,"value",item.value);valueMap.push_back(std::move(item));}
}
bool EventTrackingBinding::update(float& result) {
    float src=outVal;bool valSet=false;
    for(auto& item:valueMap) {
        if(item.id.empty() && !valSet){src=item.value;valSet=true;}
        // D tests membership before uppercasing, so lower-case IDs are skipped.
        if(item.id.size()!=1 || item.id[0]<'A' || item.id[0]>'Z')continue;
        if(ImGui::IsKeyDown(static_cast<ImGuiKey>(ImGuiKey_A+item.id[0]-'A')) || neIsEventOn(item.id[0])){src=item.value;valSet=true;break;}
    }
    if(dampenLevel==0)outVal=src;else{outVal=dampen(outVal,src,nicxlive::core::deltaTime(),static_cast<float>(11-dampenLevel));outVal=quantize(outVal);}
    result=unmapAxis(binding,outVal);return valSet;
}
std::shared_ptr<ITrackingBinding> CompoundTrackingBinding::createBinding(BindingType type) {return create(binding_,type);}
void CompoundTrackingBinding::serializeSelf(json::Object& o) const {
    o["method"]=methodNames.at(static_cast<size_t>(method));json::Array values;
    for(auto& item:bindingMap){json::Object value{{"type",bindingNames.at(static_cast<size_t>(item.type_))},{"weight",item.weight}};item.delegated->serializeSelf(value);values.push_back(std::move(value));}o["binding_map"]=std::move(values);
}
void CompoundTrackingBinding::deserializeFromJson(const json::Value& data) {
    readEnum(data,"method",method,methodNames);bindingMap.clear();
    if(auto values=json::find(data,"binding_map");values && values->is_array())for(auto& value:values->as_array()){BindingMap item;readEnum(value,"type",item.type_,bindingNames);read(value,"weight",item.weight);item.delegated=createBinding(item.type_);item.delegated->deserializeFromJson(value);bindingMap.push_back(std::move(item));}
}
bool CompoundTrackingBinding::update(float& result) {
    float sum=0,weightSum=0;
    for(auto& item:bindingMap){float src=std::numeric_limits<float>::quiet_NaN();if(!item.delegated->update(src))continue;
        switch(method){case Method::WeightedSum:weightSum+=item.weight;sum+=item.weight*src;break;case Method::WeightedMul:if(item.weight==0)continue;sum*=item.weight*src;break;case Method::Ordered:if(item.weight>weightSum){sum=src;weightSum=item.weight;}break;default:break;}}
    if(method==Method::WeightedSum && weightSum>0)sum/=weightSum;outVal=sum;result=outVal;return true;
}
}
