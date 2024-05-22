/*
    Distributed under the 2-Clause BSD License, see LICENSE file.

    Authors: Grillo del Mal
*/
module session.animation;

public import session.tracking;
import inochi2d.core.animation;
import inochi2d.core.animation.player;
import fghj;
import i18n;
import std.format;
import std.algorithm;
import session.scene;
import inmath;
import std.typecons;
import std.container.dlist;

enum TriggerType {
    None = 0,
    Tracking,
    Event
}

enum TriggerEvent {
    None = 0,
    Load = 1 << 0,
    Idle = 1 << 1,
    Sleep = 1 << 2,
}

enum ThresholdDir {
    None = 0,
    Down,
    Up,
    Both
}

const(char)* thresholdDirectionIcon(ThresholdDir dir) {
    return dir == ThresholdDir.Up ? "" :
        dir == ThresholdDir.Down ? "" :
        dir == ThresholdDir.Both ? "" :
        "";
}

const(char)* triggerTypeString(TriggerType t){
    switch(t){
        case TriggerType.None:
        return __("None");
        case TriggerType.Tracking:
        return __("Tracking");
        case TriggerType.Event:
        return __("Event");
        default:
        return __("");
    }
}

class AnimationControl {
private:
    bool playTest(float src){
        return defaultThresholds ? 
            ((inVal < 1) && (src >= 1)):
            ((playThresholdDir & ThresholdDir.Up) && (
                inVal < playThresholdValue) && (src >= playThresholdValue) ? 
                true :
                ((playThresholdDir & ThresholdDir.Down) && (
                    inVal > playThresholdValue) && (src <= playThresholdValue)) ?
                    true:
                    false);
    }

    bool stopTest(float src){
        return defaultThresholds ? 
            ((inVal > 0) && (src <= 0)):
            ((stopThresholdDir & ThresholdDir.Up) && (
                inVal < stopThresholdValue) && (src >= stopThresholdValue) ? 
                true :
                ((stopThresholdDir & ThresholdDir.Down) && (
                    inVal > stopThresholdValue) && (src <= stopThresholdValue)) ?
                    true:
                    false);
    }

    bool fullStopTest(float src){
        return defaultThresholds ? 
            ((inVal > -1) && (src <= -1)):
            ((fullStopThresholdDir & ThresholdDir.Up) && (
                inVal < fullStopThresholdValue) && (src >= fullStopThresholdValue) ? 
                true :
                ((fullStopThresholdDir & ThresholdDir.Down) && (
                    inVal > fullStopThresholdValue) && (src <= fullStopThresholdValue)) ?
                    true:
                    false);
    }

    bool playEventTest(TriggerEvent event){
        return cast(bool) (event & playEvent);
    }

    bool stopEventTest(TriggerEvent event){
        return cast(bool) (event & stopEvent);
    }

    bool fullStopEventTest(TriggerEvent event){
        return cast(bool) (event & fullStopEvent);
    }

public:
    string name;
    bool loop = true;

    TriggerType type = TriggerType.None;

    // Binding
    string sourceName;
    string sourceDisplayName;
    SourceType sourceType;

    bool defaultThresholds = true;

    float playThresholdValue = 1;
    float stopThresholdValue = 0;
    float fullStopThresholdValue = -1;

    ThresholdDir playThresholdDir = ThresholdDir.Up;
    ThresholdDir stopThresholdDir = ThresholdDir.Down;
    ThresholdDir fullStopThresholdDir = ThresholdDir.Down;

    // EventBidning
    BitFlags!TriggerEvent playEvent = TriggerEvent.None;
    BitFlags!TriggerEvent stopEvent = TriggerEvent.None;
    BitFlags!TriggerEvent fullStopEvent = TriggerEvent.None;

    // Util
    AnimationPlaybackRef anim;
    float inVal;
    TriggerEvent event = TriggerEvent.None;
    auto eventQueue = DList!TriggerEvent();
    long load_wait = 0;

    float inValToBindingValue() {
        float max_v = defaultThresholds ? 1 : max(playThresholdValue, stopThresholdValue, fullStopThresholdValue);
        float min_v = defaultThresholds ? -1 : min(playThresholdValue, stopThresholdValue, fullStopThresholdValue);
        return (inVal - min_v) / (max_v - min_v);
    }

    void serialize(S)(ref S serializer) {
        auto state = serializer.objectBegin;
            serializer.putKey("name");
            serializer.putValue(name);
            serializer.putKey("loop");
            serializer.putValue(loop);
            serializer.putKey("triggerType");
            serializer.serializeValue(type);

            switch(type) {
                case TriggerType.Tracking:
                    serializer.putKey("sourceName");
                    serializer.putValue(sourceName);
                    serializer.putKey("sourceType");
                    serializer.serializeValue(sourceType);

                    serializer.putKey("defaultThresholds");
                    serializer.putValue(defaultThresholds);

                    serializer.putKey("playThresholdValue");
                    serializer.putValue(playThresholdValue);
                    serializer.putKey("stopThresholdValue");
                    serializer.putValue(stopThresholdValue);
                    serializer.putKey("fullStopThresholdValue");
                    serializer.putValue(fullStopThresholdValue);

                    serializer.putKey("playThresholdDir");
                    serializer.serializeValue(playThresholdDir);
                    serializer.putKey("stopThresholdDir");
                    serializer.serializeValue(stopThresholdDir);
                    serializer.putKey("fullStopThresholdDir");
                    serializer.serializeValue(fullStopThresholdDir);
                    break;
                case TriggerType.Event:
                    serializer.putKey("playEvent");
                    serializer.serializeValue(cast(int) playEvent);
                    serializer.putKey("stopEvent");
                    serializer.serializeValue(cast(int) stopEvent);
                    serializer.putKey("fullStopEvent");
                    serializer.serializeValue(cast(int) fullStopEvent);
                    break;
                default: break;
            }

        serializer.objectEnd(state);
    }

    SerdeException deserializeFromFghj(Fghj data) {
        data["name"].deserializeValue(name);
        data["loop"].deserializeValue(loop);
        data["triggerType"].deserializeValue(type);

        switch(type) {
            case TriggerType.Tracking:
                data["sourceName"].deserializeValue(sourceName);
                data["sourceType"].deserializeValue(sourceType);

                data["defaultThresholds"].deserializeValue(defaultThresholds);

                data["playThresholdValue"].deserializeValue(playThresholdValue);
                data["stopThresholdValue"].deserializeValue(stopThresholdValue);
                data["fullStopThresholdValue"].deserializeValue(fullStopThresholdValue);

                data["playThresholdDir"].deserializeValue(playThresholdDir);
                data["stopThresholdDir"].deserializeValue(stopThresholdDir);
                data["fullStopThresholdDir"].deserializeValue(fullStopThresholdDir);
                this.createSourceDisplayName();
                break;
            case TriggerType.Event:
            {
                int play, stop, fullstop;
                data["playEvent"].deserializeValue(play);
                data["stopEvent"].deserializeValue(stop);
                data["fullStopEvent"].deserializeValue(fullstop);
                playEvent = cast(TriggerEvent) play;
                stopEvent = cast(TriggerEvent) stop;
                fullStopEvent = cast(TriggerEvent) fullstop;
            }
                break;
            default: break;
        }
                
        return null;
    }

    bool finalize(ref AnimationPlayer player) {
        anim = player.createOrGet(name);
        if( anim !is null){
            eventQueue.insert(TriggerEvent.Load);
            return true;
        }
        return false;
    }

    void sleep() {
        eventQueue.insert(TriggerEvent.Sleep);
    }

    void awake() {
        eventQueue.insert(TriggerEvent.Idle);
    }

    void triggerEvent() {
        eventQueue.insert(event);
    }

    void update() {
        bool eventChaged = false;

        //FIXME: Maybe a queue is too overkill...
        if(!eventQueue.empty()){
            event = eventQueue.front();
            if (event == TriggerEvent.Load){
                //HACK: First 2 frames of a newly loaded puppet last
                //      for a long time and can skip the whole animation
                //      if not looped
                if(load_wait < 3){
                    load_wait +=1;
                    event = TriggerEvent.None;
                }
                else {
                    eventQueue.removeFront();
                    eventQueue.insertFront(TriggerEvent.Idle);
                }
            }
            else {
                eventQueue.removeFront();
            }
            eventChaged = true;
        }

        switch(type) {
            case TriggerType.Event:
                // State control
                // Check if need to trigger change
                if(eventChaged){
                    if (!anim.playing || anim.paused) {
                        // Test for play
                        if(playEventTest(event)){
                            anim.play(loop);
                        } 
                    } else {
                        // Test for Stop
                        if(fullStopEventTest(event)){
                            anim.stop(true);
                        } 
                        else if(stopEventTest(event)){
                            anim.stop(false);
                        } 
                    }
                }
            break;
            case TriggerType.Tracking:
                if (sourceName.length == 0) {
                    break;
                }

                float src = 0;
                if (insScene.space.currentZone) {
                    switch(sourceType) {

                        case SourceType.Blendshape:
                            src = insScene.space.currentZone.getBlendshapeFor(sourceName);
                            break;

                        case SourceType.BonePosX:
                            src = insScene.space.currentZone.getBoneFor(sourceName).position.x;
                            break;

                        case SourceType.BonePosY:
                            src = insScene.space.currentZone.getBoneFor(sourceName).position.y;
                            break;

                        case SourceType.BonePosZ:
                            src = insScene.space.currentZone.getBoneFor(sourceName).position.z;
                            break;

                        case SourceType.BoneRotRoll:
                            src = insScene.space.currentZone.getBoneFor(sourceName).rotation.roll.degrees;
                            break;

                        case SourceType.BoneRotPitch:
                            src = insScene.space.currentZone.getBoneFor(sourceName).rotation.pitch.degrees;
                            break;

                        case SourceType.BoneRotYaw:
                            src = insScene.space.currentZone.getBoneFor(sourceName).rotation.yaw.degrees;
                            break;
                        default: assert(0);
                    }
                }

                // Stop if sleep (aka Tracking lost)
                if (eventChaged){
                    if (event == TriggerEvent.Sleep) {
                        anim.stop(true);
                    }
                }

                // Ignore if not idle.
                if (event != TriggerEvent.Idle) {
                    break;
                }

                // Check if need to trigger change
                if (!anim.playing || anim.paused) {
                    // Test for play
                    if(playTest(src)) anim.play(loop);
                } else {
                    // Test for Stop
                    if(fullStopTest(src)) anim.stop(true);
                    else if(stopTest(src)) anim.stop(false);
                }

                //Set latest inVal
                inVal = src;
                break;
            default: break;
        }
    }

    void createSourceDisplayName() {
        switch(sourceType) {
            case SourceType.Blendshape:
                sourceDisplayName = sourceName;
                break;
            case SourceType.BonePosX:
                sourceDisplayName = _("%s (X)").format(sourceName);
                break;
            case SourceType.BonePosY:
                sourceDisplayName = _("%s (Y)").format(sourceName);
                break;
            case SourceType.BonePosZ:
                sourceDisplayName = _("%s (Z)").format(sourceName);
                break;
            case SourceType.BoneRotRoll:
                sourceDisplayName = _("%s (Roll)").format(sourceName);
                break;
            case SourceType.BoneRotPitch:
                sourceDisplayName = _("%s (Pitch)").format(sourceName);
                break;
            case SourceType.BoneRotYaw:
                sourceDisplayName = _("%s (Yaw)").format(sourceName);
                break;
            default: assert(0);    
        }
    }

}
