/*
    Distributed under the 2-Clause BSD License, see LICENSE file.

    Authors: Grillo del Mal
*/
module session.panels.animations;
import inui.panel;
import i18n;
import session.scene;
import inui.widgets;
import std.string;
import std.algorithm.searching;
import inochi2d.core.animation.player;
import inmath;
import session.animation;
import bindbc.imgui;

private {
    string trackingFilter;

    struct TrackingSource {
        bool isBone;
        string name;
        const(char)* cName;
    }
}

class AnimationsPanel : Panel {
private:
    TrackingSource[] sources;
    string[] indexableSourceNames;

    // Refreshes the list of tracking sources
    void refresh(ref AnimationControl[] animationControls) {
        auto blendshapes = insScene.space.getAllBlendshapeNames();
        auto bones = insScene.space.getAllBoneNames();
        
        sources.length = blendshapes.length + bones.length;
        indexableSourceNames.length = sources.length;

        foreach(i, blendshape; blendshapes) {
            sources[i] = TrackingSource(
                false,
                blendshape,
                blendshape.toStringz
            );
            indexableSourceNames[i] = blendshape.toLower;
        }

        foreach(i, bone; bones) {
            sources[blendshapes.length+i] = TrackingSource(
                true,
                bone,
                bone.toStringz
            );

            indexableSourceNames[blendshapes.length+i] = bone.toLower;
        }

        // Add any bindings unnacounted for which are stored in the model.
        trkMain: foreach(ac; animationControls) {
            
            // Skip non-existent sources
            if (ac.sourceName.length == 0) continue;

            TrackingSource src = TrackingSource(
                ac.sourceType != SourceType.Blendshape,
                ac.sourceName,
                ac.sourceName.toStringz
            );

            // Skip anything we already know
            foreach(xsrc; sources) {
                if (xsrc.isBone == src.isBone && xsrc.name == src.name) continue trkMain;
            }

            sources ~= src;
            indexableSourceNames ~= src.name.toLower;
        }
    }

    void trackingOptions(AnimationControl ac){
        float default_val;
        int default_hold_val = 0;
        bool hasTrackingSrc = ac.sourceName.length > 0;
        uiImIndent();

        uiImLabel(_("Tracking Parameter"));
        if (uiImBeginComboBox("SELECTION_COMBO", hasTrackingSrc ? ac.sourceDisplayName.toStringz : __("Not tracked"))) {
            if (uiImInputText("###FILTER", uiImAvailableSpace().x, trackingFilter)) {
                trackingFilter = trackingFilter.toLower();
            }

            uiImDummy(vec2(0, 8));

            foreach(ix, source; sources) {
                
                if (trackingFilter.length > 0 && !indexableSourceNames[ix].canFind(trackingFilter)) continue;

                bool selected = ac.sourceName == source.name;
                bool nameValid = source.name.length > 0;
                if (source.isBone) {
                    if (uiImBeginMenu(source.cName)) {
                        if (uiImMenuItem(__("X"))) {
                            ac.sourceName = source.name;
                            ac.sourceType = SourceType.BonePosX;
                            ac.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Y"))) {
                            ac.sourceName = source.name;
                            ac.sourceType = SourceType.BonePosY;
                            ac.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Z"))) {
                            ac.sourceName = source.name;
                            ac.sourceType = SourceType.BonePosZ;
                            ac.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Roll"))) {
                            ac.sourceName = source.name;
                            ac.sourceType = SourceType.BoneRotRoll;
                            ac.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Pitch"))) {
                            ac.sourceName = source.name;
                            ac.sourceType = SourceType.BoneRotPitch;
                            ac.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Yaw"))) {
                            ac.sourceName = source.name;
                            ac.sourceType = SourceType.BoneRotYaw;
                            ac.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        uiImEndMenu();
                    }
                } else {
                    if (uiImSelectable(nameValid ? source.cName : "###NoName", selected)) {
                        trackingFilter = null;
                        ac.sourceType = SourceType.Blendshape;
                        ac.sourceName = source.name;
                        ac.createSourceDisplayName();
                    }
                }
            }
            uiImEndComboBox();
        }

        if (hasTrackingSrc) {
            uiImSameLine(0, 4);
            if (uiImButton(__("Reset"))){
                ac.sourceName = null;
            }
        }

        if (hasTrackingSrc) {
            uiImProgress(ac.inValToBindingValue(), vec2(-float.min_normal, 0), "");
            uiImDummy(vec2(0, 8));
            uiImCheckbox(__("Default Thresholds"), ac.defaultThresholds);
            uiImSameLine(0, 0);
            uiImDummy(vec2(10, 5));
            uiImSameLine(0, 0);
            uiImCheckbox(__("Use Hold Delay"), ac.useHoldDelay);

            if(ac.defaultThresholds){
                igBeginDisabled();
            }
            uiImLabel(_("Play Threshold"));
            uiImPush(0);
                uiImIndent();
                default_val = 1;
                switch(ac.sourceType) {
                    case SourceType.Blendshape:
                        // TODO: Make all blendshapes in facetrack-d 0->1
                        uiImDrag(ac.defaultThresholds ? default_val : ac.playThresholdValue, -1, 1);
                        break;

                    case SourceType.BonePosX:
                    case SourceType.BonePosY:
                    case SourceType.BonePosZ:
                        uiImDrag(ac.defaultThresholds ? default_val : ac.playThresholdValue, -float.max, float.max);
                        break;

                    case SourceType.BoneRotPitch:
                    case SourceType.BoneRotRoll:
                    case SourceType.BoneRotYaw:
                        uiImDrag(ac.defaultThresholds ? default_val : ac.playThresholdValue, -180, 180);
                        break;
                        
                    default: assert(0);
                }
                uiImSameLine(0, 0);
                if (uiImButton(
                        thresholdDirectionIcon(ac.defaultThresholds ? ThresholdDir.Up : ac.playThresholdDir))) {
                    if(ac.playThresholdDir < ThresholdDir.Both) ac.playThresholdDir += 1;
                    else ac.playThresholdDir = ThresholdDir.None;
                }
                if(ac.useHoldDelay){
                    if(ac.playThresholdDir == ThresholdDir.None || ac.playThresholdDir == ThresholdDir.Both){
                        igBeginDisabled();
                    }
                    uiImLabel(_("Hold Delay (ms):"));
                    uiImPush(0);
                    uiImDrag(
                        ac.defaultThresholds || ac.playThresholdDir == ThresholdDir.None || ac.playThresholdDir == ThresholdDir.Both ? 
                            default_hold_val : ac.playThresholdHoldDelay, 0, int.max);
                    uiImPop();
                    if(ac.playThresholdDir == ThresholdDir.None || ac.playThresholdDir == ThresholdDir.Both){
                        igEndDisabled();
                    }
                }

                uiImUnindent();
            uiImPop();
            
            uiImLabel(_("Stop Threshold"));
            uiImPush(1);
                uiImIndent();
                default_val = 0;
                switch(ac.sourceType) {
                    case SourceType.Blendshape:
                        // TODO: Make all blendshapes in facetrack-d 0->1
                        uiImDrag(ac.defaultThresholds ? default_val : ac.stopThresholdValue, -1, 1);
                        break;

                    case SourceType.BonePosX:
                    case SourceType.BonePosY:
                    case SourceType.BonePosZ:
                        uiImDrag(ac.defaultThresholds ? default_val : ac.stopThresholdValue, -float.max, float.max);
                        break;

                    case SourceType.BoneRotPitch:
                    case SourceType.BoneRotRoll:
                    case SourceType.BoneRotYaw:
                        uiImDrag(ac.defaultThresholds ? default_val : ac.stopThresholdValue, -180, 180);
                        break;
                        
                    default: assert(0);
                }
                uiImSameLine(0, 0);
                if (uiImButton(
                        thresholdDirectionIcon(ac.defaultThresholds ? ThresholdDir.Down : ac.stopThresholdDir))) {
                    if(ac.stopThresholdDir < ThresholdDir.Both) ac.stopThresholdDir += 1;
                    else ac.stopThresholdDir = ThresholdDir.None;
                }
                if(ac.useHoldDelay){
                    if(ac.stopThresholdDir == ThresholdDir.None || ac.stopThresholdDir == ThresholdDir.Both){
                        igBeginDisabled();
                    }
                    uiImLabel(_("Hold Delay (ms):"));
                    uiImPush(0);
                    uiImDrag(
                        ac.defaultThresholds || ac.stopThresholdDir == ThresholdDir.None || ac.stopThresholdDir == ThresholdDir.Both ? 
                            default_hold_val : ac.stopThresholdHoldDelay, 0, int.max);
                    uiImPop();
                    if(ac.stopThresholdDir == ThresholdDir.None || ac.stopThresholdDir == ThresholdDir.Both){
                        igEndDisabled();
                    }
                }
                uiImUnindent();
            uiImPop();

            uiImLabel(_("Full Stop Threshold"));
            uiImPush(2);
                uiImIndent();
                default_val = -1;
                switch(ac.sourceType) {
                    case SourceType.Blendshape:
                        // TODO: Make all blendshapes in facetrack-d 0->1
                        uiImDrag(ac.defaultThresholds ? default_val : ac.fullStopThresholdValue, -1, 1);
                        break;

                    case SourceType.BonePosX:
                    case SourceType.BonePosY:
                    case SourceType.BonePosZ:
                        uiImDrag(ac.defaultThresholds ? default_val : ac.fullStopThresholdValue, -float.max, float.max);
                        break;

                    case SourceType.BoneRotPitch:
                    case SourceType.BoneRotRoll:
                    case SourceType.BoneRotYaw:
                        uiImDrag(ac.defaultThresholds ? default_val : ac.fullStopThresholdValue, -180, 180);
                        break;
                        
                    default: assert(0);
                }
                uiImSameLine(0, 0);
                if (uiImButton(
                        thresholdDirectionIcon(ac.defaultThresholds ? ThresholdDir.Down : ac.fullStopThresholdDir))) {
                    if(ac.fullStopThresholdDir < ThresholdDir.Both) ac.fullStopThresholdDir += 1;
                    else ac.fullStopThresholdDir = ThresholdDir.None;
                }
                if(ac.useHoldDelay){
                    if(ac.fullStopThresholdDir == ThresholdDir.None || ac.fullStopThresholdDir == ThresholdDir.Both){
                        igBeginDisabled();
                    }
                    uiImLabel(_("Hold Delay (ms):"));
                    uiImPush(0);
                    uiImDrag(
                        ac.defaultThresholds || ac.fullStopThresholdDir == ThresholdDir.None || ac.fullStopThresholdDir == ThresholdDir.Both ? 
                            default_hold_val : ac.fullStopThresholdHoldDelay, 0, int.max);
                    uiImPop();
                    if(ac.fullStopThresholdDir == ThresholdDir.None || ac.fullStopThresholdDir == ThresholdDir.Both){
                        igEndDisabled();
                    }
                }
                uiImUnindent();
            uiImPop();
            if(ac.defaultThresholds){
                igEndDisabled();
            }

        }
        uiImUnindent();
    }

    void eventOptions(AnimationControl ac){
        uiImIndent();
            igBeginTable(__("Event"), 3, ImGuiTableFlags.SizingStretchProp);

            igTableNextColumn();
            uiImLabel(_("Play on"));
            igTableNextColumn();
            uiImLabel(_("Stop on"));
            igTableNextColumn();
            uiImLabel(_("Full stop on"));
            igTableNextColumn();

            uiImIndent();
            uiImPush(0);
            {
                bool load = ac.playEvent.Load;
                bool idle = ac.playEvent.Idle;
                bool sleep = ac.playEvent.Sleep;
                if(uiImCheckbox(__("Load"), load)){
                    ac.playEvent.Load = load;
                    ac.stopEvent.Load = false;
                    ac.fullStopEvent.Load = false;
                    ac.triggerEvent();
                }
                if(uiImCheckbox(__("Idle"), idle)){
                    ac.playEvent.Idle = idle;
                    if(ac.playEvent.Idle){
                        ac.stopEvent.Idle = false;
                        ac.fullStopEvent.Idle = false;
                    }
                    ac.triggerEvent();
                }
                if(uiImCheckbox(__("Sleep"), sleep)){
                    ac.playEvent.Sleep = sleep;
                    if(ac.playEvent.Sleep){
                        ac.stopEvent.Sleep = false;
                        ac.fullStopEvent.Sleep = false;
                    }
                    ac.triggerEvent();
                }
            }
            uiImPop();
            uiImUnindent();

            igTableNextColumn();

            uiImIndent();
            uiImPush(1);
            {
                bool idle = ac.stopEvent.Idle;
                bool sleep = ac.stopEvent.Sleep;
                uiImDummy(vec2(0, 20));
                if(ac.playEvent.Idle) igBeginDisabled();
                if(uiImCheckbox(__("Idle"), idle)){
                    ac.stopEvent.Idle = idle;
                    ac.triggerEvent();
                }
                if(ac.playEvent.Idle) igEndDisabled();
                if(ac.playEvent.Sleep) igBeginDisabled();
                if(uiImCheckbox(__("Sleep"), sleep)){
                    ac.stopEvent.Sleep = sleep;
                    ac.triggerEvent();
                } 
                if(ac.playEvent.Sleep) igEndDisabled();
            }
            uiImPop();
            uiImUnindent();

            igTableNextColumn();

            uiImIndent();
            uiImPush(2);
            {
                bool idle = ac.fullStopEvent.Idle;
                bool sleep = ac.fullStopEvent.Sleep;
                uiImDummy(vec2(0, 20));
                if(ac.playEvent.Idle) igBeginDisabled();
                if(uiImCheckbox(__("Idle"), idle)){
                    ac.fullStopEvent.Idle = idle;
                    ac.triggerEvent();
                }
                if(ac.playEvent.Idle) igEndDisabled();
                if(ac.playEvent.Sleep) igBeginDisabled();
                if(uiImCheckbox(__("Sleep"), sleep)){
                    ac.fullStopEvent.Sleep = sleep;
                    ac.triggerEvent();
                }
                if(ac.playEvent.Sleep) igEndDisabled();
            }
            uiImPop();
            uiImUnindent();

            igEndTable();
        uiImUnindent();
        
    }
protected:

    override 
    void onUpdate() {
        auto item = insSceneSelectedSceneItem();
        if (item) {

            if (uiImButton(__("Refresh"))) {
                insScene.space.refresh();
                refresh(item.animations);
            }

            uiImSameLine(0, 4);

            if (uiImButton(__("Save to File"))) {
                try {
                    item.saveAnimations();
                } catch (Exception ex) {
                    uiImDialog(__("Error"), ex.msg);
                }
            }

            foreach(ref ac; item.animations) {
                uiImPush(&ac);
                auto anim = ac.anim;
                if (uiImHeader(ac.name.toStringz, true)) {

                    if (uiImButton("")) {
                        anim.stop(igIsKeyDown(ImGuiKey.LeftShift) || igIsKeyDown(ImGuiKey.RightShift));
                    }
                    uiImSameLine(0, 0);
                    
                    if (uiImButton(anim && !(!anim.playing || anim.paused) ? "" : "")) {
                        if (!anim.playing || anim.paused) anim.play(ac.loop);
                        else anim.pause();
                    }
                    uiImSameLine(0, 0);
                    uiImProgress((cast(float)anim.frame) / anim.frames, vec2(-float.min_normal, 0), "");

                    uiImCheckbox(__("Loop"), ac.loop);

                    uiImDummy(vec2(0, 12));

                    uiImLabel(_("Trigger"));
                    if (uiImBeginComboBox("ACType", triggerTypeString(ac.type))) {
                        if (uiImSelectable(triggerTypeString(TriggerType.None))) {
                            ac.type = TriggerType.None;
                        }
                        if (uiImSelectable(triggerTypeString(TriggerType.Tracking))) {
                            ac.type = TriggerType.Tracking;
                        }
                        if (uiImSelectable(triggerTypeString(TriggerType.Event))) {
                            ac.type = TriggerType.Event;
                        }
                        uiImEndComboBox();
                    }

                    uiImDummy(vec2(0, 8));

                    switch(ac.type) {
                        case TriggerType.Tracking:
                            trackingOptions(ac);
                            break;
                        case TriggerType.Event:
                            eventOptions(ac);
                            break;
                        default: break;
                    }

                    uiImDummy(vec2(0, 8));

                }
                uiImPop();
            }
        } else {
            uiImLabel(_("No puppet selected"));
        }
    }

public:
    this() {
        super("Animations", _("Animations"), true);
    }
}

mixin inPanel!AnimationsPanel;