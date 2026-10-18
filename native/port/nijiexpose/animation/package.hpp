#pragma once
#include "nijiexpose/tracking/package.hpp"
#include "nijilive/animation/player.hpp"
#include <core/timing.hpp>
#include <deque>
namespace nijiexpose::animation {
enum class TriggerType {None,Tracking,Event};
enum class TriggerEvent {None=0,Load=1,Idle=2,Sleep=4};
enum class ThresholdDir {None,Down,Up,Both};
inline int operator&(ThresholdDir a,ThresholdDir b){return int(a)&int(b);}
inline int operator&(TriggerEvent a,int b){return int(a)&b;}
const char* thresholdDirectionIcon(ThresholdDir dir);
const char* triggerTypeString(TriggerType type);
class AnimationControl {
    #include "thresholds.inc"
public:
    std::string name;
    bool loop=true;
    TriggerType type=TriggerType::None;
    std::string sourceName,sourceDisplayName;
    tracking::SourceType sourceType=tracking::SourceType::Blendshape;
    bool defaultThresholds=true,useHoldDelay=false;
    float playThresholdValue=1,stopThresholdValue=0,fullStopThresholdValue=-1;
    ThresholdDir playThresholdDir=ThresholdDir::Up,stopThresholdDir=ThresholdDir::Down,fullStopThresholdDir=ThresholdDir::Down;
    int playThresholdHoldDelay=0,stopThresholdHoldDelay=0,fullStopThresholdHoldDelay=0;
    int playThresholdHoldTime=0,stopThresholdHoldTime=0,fullStopThresholdHoldTime=0;
    int playEvent=0,stopEvent=0,fullStopEvent=0;
    AnimationPlaybackRef anim;
    float inVal=std::numeric_limits<float>::quiet_NaN();
    TriggerEvent event=TriggerEvent::None;
    std::deque<TriggerEvent> eventQueue;
    int64_t load_wait=0;
    float inValToBindingValue() const;
    json::Value serialize() const;
    void deserializeFromJson(const json::Value& data);
    bool finalize(AnimationPlayer& player);
    void sleep(){eventQueue.push_back(TriggerEvent::Sleep);}
    void awake(){eventQueue.push_back(TriggerEvent::Idle);}
    void triggerEvent(){eventQueue.push_back(event);}
    void update();
    void createSourceDisplayName();
};
}
