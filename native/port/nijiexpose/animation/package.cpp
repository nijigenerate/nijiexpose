#include "package.hpp"
#include "nijiexpose/tracking/context.hpp"
#include "i18n/tr.hpp"
#include <numbers>
namespace nijiexpose::animation {
namespace {
constexpr std::array<const char*,3> triggers={"None","Tracking","Event"};
constexpr std::array<const char*,4> directions={"None","Down","Up","Both"};
constexpr std::array<const char*,8> sources={"Blendshape","BonePosX","BonePosY","BonePosZ","BoneRotRoll","BoneRotPitch","BoneRotYaw","KeyPress"};
template<class T>void read(const json::Value& data,const char* name,T& value){if(auto v=json::find(data,name))json::deserialize(*v,value);}
template<class E,size_t N>void readEnum(const json::Value& data,const char* name,E& out,const std::array<const char*,N>& names){if(auto v=json::find(data,name);v&&v->is_string())for(size_t i=0;i<N;++i)if(v->as_string()==names[i]){out=static_cast<E>(i);break;}}
}
const char* thresholdDirectionIcon(ThresholdDir dir){return dir==ThresholdDir::Up?"":dir==ThresholdDir::Down?"":dir==ThresholdDir::Both?"":"";}
const char* triggerTypeString(TriggerType type){switch(type){case TriggerType::None:return i18n::__("None");case TriggerType::Tracking:return i18n::__("Tracking");case TriggerType::Event:return i18n::__("Event");default:return i18n::__("");}}
float AnimationControl::inValToBindingValue() const {
    float max_v=defaultThresholds?1:std::max({playThresholdValue,stopThresholdValue,fullStopThresholdValue});
    float min_v=defaultThresholds?-1:std::min({playThresholdValue,stopThresholdValue,fullStopThresholdValue});
    return (inVal-min_v)/(max_v-min_v);
}
json::Value AnimationControl::serialize() const {
    json::Object o{{"name",name},{"loop",loop},{"triggerType",triggers.at(static_cast<size_t>(type))}};
    if(type==TriggerType::Tracking){
        o["sourceName"]=sourceName;o["sourceType"]=sources.at(static_cast<size_t>(sourceType));
        #define FIELD(name) o[#name]=name
        FIELD(defaultThresholds);FIELD(useHoldDelay);FIELD(playThresholdValue);FIELD(stopThresholdValue);FIELD(fullStopThresholdValue);FIELD(playThresholdHoldDelay);FIELD(stopThresholdHoldDelay);FIELD(fullStopThresholdHoldDelay);
        #undef FIELD
        o["playThresholdDir"]=directions.at(static_cast<size_t>(playThresholdDir));o["stopThresholdDir"]=directions.at(static_cast<size_t>(stopThresholdDir));o["fullStopThresholdDir"]=directions.at(static_cast<size_t>(fullStopThresholdDir));
    } else if(type==TriggerType::Event){o["playEvent"]=playEvent;o["stopEvent"]=stopEvent;o["fullStopEvent"]=fullStopEvent;}
    return o;
}
void AnimationControl::deserializeFromJson(const json::Value& data){
    read(data,"name",name);read(data,"loop",loop);readEnum(data,"triggerType",type,triggers);
    if(type==TriggerType::Tracking){
        read(data,"sourceName",sourceName);readEnum(data,"sourceType",sourceType,sources);
        #define FIELD(name) read(data,#name,name)
        FIELD(defaultThresholds);FIELD(useHoldDelay);FIELD(playThresholdValue);FIELD(stopThresholdValue);FIELD(fullStopThresholdValue);FIELD(playThresholdHoldDelay);FIELD(stopThresholdHoldDelay);FIELD(fullStopThresholdHoldDelay);
        #undef FIELD
        readEnum(data,"playThresholdDir",playThresholdDir,directions);readEnum(data,"stopThresholdDir",stopThresholdDir,directions);readEnum(data,"fullStopThresholdDir",fullStopThresholdDir,directions);createSourceDisplayName();
    }else if(type==TriggerType::Event){playEvent=stopEvent=fullStopEvent=0;read(data,"playEvent",playEvent);read(data,"stopEvent",stopEvent);read(data,"fullStopEvent",fullStopEvent);}
}
bool AnimationControl::finalize(AnimationPlayer& player){anim=player.createOrGet(name);if(anim){eventQueue.push_back(TriggerEvent::Load);return true;}return false;}
void AnimationControl::update(){
    bool eventChaged=false;
    if(!eventQueue.empty()){
        event=eventQueue.front();
        if(event==TriggerEvent::Load){if(load_wait<3){++load_wait;event=TriggerEvent::None;}else{eventQueue.pop_front();eventQueue.push_front(TriggerEvent::Idle);}}
        else eventQueue.pop_front();eventChaged=true;
    }
    switch(type){
    case TriggerType::Event:
        if(eventChaged){if(!anim->playing()||anim->paused()){if(playEventTest(event))anim->play(loop);}else{if(fullStopEventTest(event))anim->stop(true);else if(stopEventTest(event))anim->stop(false);}}
        break;
    case TriggerType::Tracking:{
        if(sourceName.empty())break;
        float src=0;auto zone=tracking::insCurrentTrackingSpace()->currentZone();
        if(zone)switch(sourceType){
            case tracking::SourceType::Blendshape:src=zone->getBlendshapeFor(sourceName);break;
            case tracking::SourceType::BonePosX:src=zone->getBoneFor(sourceName).position.x;break;
            case tracking::SourceType::BonePosY:src=zone->getBoneFor(sourceName).position.y;break;
            case tracking::SourceType::BonePosZ:src=zone->getBoneFor(sourceName).position.z;break;
            case tracking::SourceType::BoneRotRoll:src=static_cast<float>((180/std::numbers::pi)*zone->getBoneFor(sourceName).rotation.roll());break;
            case tracking::SourceType::BoneRotPitch:src=static_cast<float>((180/std::numbers::pi)*zone->getBoneFor(sourceName).rotation.pitch());break;
            case tracking::SourceType::BoneRotYaw:src=static_cast<float>((180/std::numbers::pi)*zone->getBoneFor(sourceName).rotation.yaw());break;
            default:throw std::logic_error("D animation source assertion");
        }
        if(eventChaged && event==TriggerEvent::Sleep)anim->stop(true);
        if(event!=TriggerEvent::Idle)break;
        updateHold(src);
        if(!anim->playing()||anim->paused()){if(!useHoldDelay||playThresholdHoldDelay==0?playTest(src):playHoldTest(src))anim->play(loop);}
        else{if(!useHoldDelay||fullStopThresholdHoldDelay==0?fullStopTest(src):fullStopHoldTest(src))anim->stop(true);else if(!useHoldDelay||stopThresholdHoldDelay==0?stopTest(src):stopHoldTest(src))anim->stop(false);}
        inVal=src;break;
    }
    default:break;
    }
}
void AnimationControl::createSourceDisplayName(){
    if(sourceType==tracking::SourceType::KeyPress)throw std::logic_error("D animation source assertion");
    tracking::RatioTrackingBinding binding(nullptr);binding.sourceType=sourceType;binding.sourceName=sourceName;binding.createSourceDisplayName();sourceDisplayName=binding.sourceDisplayName;
}
}
