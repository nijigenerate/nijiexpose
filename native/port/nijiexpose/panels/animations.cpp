// Direct translation of dlang_archive/source/nijiexpose/panels/animations.d.
#include "factory.hpp"
#include "nijiexpose/scene/package.hpp"
#include "nijiui/widgets/input.hpp"
#include "nijiui/widgets/dialog.hpp"
#include "dcompat/case.hpp"
#include "i18n/tr.hpp"
#include <cassert>
namespace nijiexpose::panels {
using namespace nijiui;using namespace animation;using namespace tracking;using namespace scene;
using i18n::_;using i18n::__;
namespace {thread_local std::string trackingFilter;
struct TrackingSource{bool isBone;std::string name;};
bool hasEvent(int bits,TriggerEvent flag){return (bits&static_cast<int>(flag))!=0;}
void setEvent(int& bits,TriggerEvent flag,bool value){auto mask=static_cast<int>(flag);if(value)bits|=mask;else bits&=~mask;}
}
class AnimationsPanel : public Panel {
private:
    std::vector<TrackingSource> sources;
    std::vector<std::string> indexableSourceNames;

    // Refreshes the list of tracking sources
    void refresh(const std::vector<std::shared_ptr<AnimationControl>>& animationControls) {
        auto blendshapes = insScene()->space->getAllBlendshapeNames();
        auto bones = insScene()->space->getAllBoneNames();
        
        sources.resize(blendshapes.size() + bones.size());
        indexableSourceNames.resize(sources.size());

        for(size_t i=0;i<blendshapes.size();++i){auto& blendshape=blendshapes[i];
            sources[i] = TrackingSource(
                false,
                blendshape
            );
            indexableSourceNames[i] = dcompat::toLower(blendshape);
        }

        for(size_t i=0;i<bones.size();++i){auto& bone=bones[i];
            sources[blendshapes.size()+i] = TrackingSource(
                true,
                bone
            );

            indexableSourceNames[blendshapes.size()+i] = dcompat::toLower(bone);
        }

        // Add any bindings unnacounted for which are stored in the model.
        for(auto& ac:animationControls) {
            
            // Skip non-existent sources
            if (ac->sourceName.empty()) continue;

            TrackingSource src = TrackingSource(
                ac->sourceType != SourceType::Blendshape,
                ac->sourceName
            );

            // Skip anything we already know
            if(std::any_of(sources.begin(),sources.end(),[&](auto& xsrc){return xsrc.isBone==src.isBone && xsrc.name==src.name;}))continue;

            sources.push_back(src);
            indexableSourceNames.push_back(dcompat::toLower(src.name));
        }
    }

    void trackingOptions(const std::shared_ptr<AnimationControl>& ac){
        float default_val=std::numeric_limits<float>::quiet_NaN();
        int default_hold_val = 0;
        bool hasTrackingSrc = !ac->sourceName.empty();
        uiImIndent();

        uiImLabel(_("Tracking Parameter"));
        if (uiImBeginComboBox("SELECTION_COMBO", hasTrackingSrc ? ac->sourceDisplayName.c_str() : __("Not tracked"))) {
            std::string filter = trackingFilter;
            if (uiImInputText("###FILTER", uiImAvailableSpace().x, filter)) {
                trackingFilter = std::string(dcompat::fromStringz(dcompat::toLower(filter)));
            }

            uiImDummy(vec2(0, 8));

            for(size_t ix=0;ix<sources.size();++ix){auto& source=sources[ix];
                if (!trackingFilter.empty() && indexableSourceNames[ix].find(trackingFilter)==std::string::npos) continue;

                bool selected = ac->sourceName == source.name;
                bool nameValid = !source.name.empty();
                if (source.isBone) {
                    if (uiImBeginMenu(source.name.c_str())) {
                        if (uiImMenuItem(__("X"))) {
                            ac->sourceName = source.name;
                            ac->sourceType = SourceType::BonePosX;
                            ac->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        if (uiImMenuItem(__("Y"))) {
                            ac->sourceName = source.name;
                            ac->sourceType = SourceType::BonePosY;
                            ac->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        if (uiImMenuItem(__("Z"))) {
                            ac->sourceName = source.name;
                            ac->sourceType = SourceType::BonePosZ;
                            ac->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        if (uiImMenuItem(__("Roll"))) {
                            ac->sourceName = source.name;
                            ac->sourceType = SourceType::BoneRotRoll;
                            ac->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        if (uiImMenuItem(__("Pitch"))) {
                            ac->sourceName = source.name;
                            ac->sourceType = SourceType::BoneRotPitch;
                            ac->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        if (uiImMenuItem(__("Yaw"))) {
                            ac->sourceName = source.name;
                            ac->sourceType = SourceType::BoneRotYaw;
                            ac->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        uiImEndMenu();
                    }
                } else {
                    if (uiImSelectable(nameValid ? source.name.c_str() : "###NoName", selected)) {
                        trackingFilter.clear();
                        ac->sourceType = SourceType::Blendshape;
                        ac->sourceName = source.name;
                        ac->createSourceDisplayName();
                    }
                }
            }
            uiImEndComboBox();
        }

        if (hasTrackingSrc) {
            uiImSameLine(0, 4);
            if (uiImButton(__("Reset"))){
                ac->sourceName.clear();
            }
        }

        if (hasTrackingSrc) {
            uiImProgress(ac->inValToBindingValue(), vec2(-std::numeric_limits<float>::min(), 0), "");
            uiImDummy(vec2(0, 8));
            uiImCheckbox(__("Default Thresholds"), ac->defaultThresholds);
            uiImSameLine(0, 0);
            uiImDummy(vec2(10, 5));
            uiImSameLine(0, 0);
            uiImCheckbox(__("Use Hold Delay"), ac->useHoldDelay);

            if(ac->defaultThresholds){
                ImGui::BeginDisabled();
            }
            uiImLabel(_("Play Threshold"));
            uiImPush(0);
                uiImIndent();
                default_val = 1;
                switch(ac->sourceType) {
                    case SourceType::Blendshape:
                        // TODO: Make all blendshapes in facetrack-d 0->1
                        uiImDrag(ac->defaultThresholds ? default_val : ac->playThresholdValue, -1, 1);
                        break;

                    case SourceType::BonePosX:
                    case SourceType::BonePosY:
                    case SourceType::BonePosZ:
                        uiImDrag(ac->defaultThresholds ? default_val : ac->playThresholdValue, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
                        break;

                    case SourceType::BoneRotPitch:
                    case SourceType::BoneRotRoll:
                    case SourceType::BoneRotYaw:
                        uiImDrag(ac->defaultThresholds ? default_val : ac->playThresholdValue, -180, 180);
                        break;
                        
                    default: assert(0);
                }
                uiImSameLine(0, 0);
                if (uiImButton(
                        thresholdDirectionIcon(ac->defaultThresholds ? ThresholdDir::Up : ac->playThresholdDir))) {
                    if(ac->playThresholdDir < ThresholdDir::Both) ac->playThresholdDir = static_cast<ThresholdDir>(static_cast<int>(ac->playThresholdDir)+1);
                    else ac->playThresholdDir = ThresholdDir::None;
                }
                if(ac->useHoldDelay){
                    if(ac->playThresholdDir == ThresholdDir::None || ac->playThresholdDir == ThresholdDir::Both){
                        ImGui::BeginDisabled();
                    }
                    uiImLabel(_("Hold Delay (ms):"));
                    uiImPush(0);
                    uiImDrag(
                        ac->defaultThresholds || ac->playThresholdDir == ThresholdDir::None || ac->playThresholdDir == ThresholdDir::Both ? 
                            default_hold_val : ac->playThresholdHoldDelay, 0, std::numeric_limits<int>::max());
                    uiImPop();
                    if(ac->playThresholdDir == ThresholdDir::None || ac->playThresholdDir == ThresholdDir::Both){
                        ImGui::EndDisabled();
                    }
                }

                uiImUnindent();
            uiImPop();
            
            uiImLabel(_("Stop Threshold"));
            uiImPush(1);
                uiImIndent();
                default_val = 0;
                switch(ac->sourceType) {
                    case SourceType::Blendshape:
                        // TODO: Make all blendshapes in facetrack-d 0->1
                        uiImDrag(ac->defaultThresholds ? default_val : ac->stopThresholdValue, -1, 1);
                        break;

                    case SourceType::BonePosX:
                    case SourceType::BonePosY:
                    case SourceType::BonePosZ:
                        uiImDrag(ac->defaultThresholds ? default_val : ac->stopThresholdValue, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
                        break;

                    case SourceType::BoneRotPitch:
                    case SourceType::BoneRotRoll:
                    case SourceType::BoneRotYaw:
                        uiImDrag(ac->defaultThresholds ? default_val : ac->stopThresholdValue, -180, 180);
                        break;
                        
                    default: assert(0);
                }
                uiImSameLine(0, 0);
                if (uiImButton(
                        thresholdDirectionIcon(ac->defaultThresholds ? ThresholdDir::Down : ac->stopThresholdDir))) {
                    if(ac->stopThresholdDir < ThresholdDir::Both) ac->stopThresholdDir = static_cast<ThresholdDir>(static_cast<int>(ac->stopThresholdDir)+1);
                    else ac->stopThresholdDir = ThresholdDir::None;
                }
                if(ac->useHoldDelay){
                    if(ac->stopThresholdDir == ThresholdDir::None || ac->stopThresholdDir == ThresholdDir::Both){
                        ImGui::BeginDisabled();
                    }
                    uiImLabel(_("Hold Delay (ms):"));
                    uiImPush(0);
                    uiImDrag(
                        ac->defaultThresholds || ac->stopThresholdDir == ThresholdDir::None || ac->stopThresholdDir == ThresholdDir::Both ? 
                            default_hold_val : ac->stopThresholdHoldDelay, 0, std::numeric_limits<int>::max());
                    uiImPop();
                    if(ac->stopThresholdDir == ThresholdDir::None || ac->stopThresholdDir == ThresholdDir::Both){
                        ImGui::EndDisabled();
                    }
                }
                uiImUnindent();
            uiImPop();

            uiImLabel(_("Full Stop Threshold"));
            uiImPush(2);
                uiImIndent();
                default_val = -1;
                switch(ac->sourceType) {
                    case SourceType::Blendshape:
                        // TODO: Make all blendshapes in facetrack-d 0->1
                        uiImDrag(ac->defaultThresholds ? default_val : ac->fullStopThresholdValue, -1, 1);
                        break;

                    case SourceType::BonePosX:
                    case SourceType::BonePosY:
                    case SourceType::BonePosZ:
                        uiImDrag(ac->defaultThresholds ? default_val : ac->fullStopThresholdValue, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
                        break;

                    case SourceType::BoneRotPitch:
                    case SourceType::BoneRotRoll:
                    case SourceType::BoneRotYaw:
                        uiImDrag(ac->defaultThresholds ? default_val : ac->fullStopThresholdValue, -180, 180);
                        break;
                        
                    default: assert(0);
                }
                uiImSameLine(0, 0);
                if (uiImButton(
                        thresholdDirectionIcon(ac->defaultThresholds ? ThresholdDir::Down : ac->fullStopThresholdDir))) {
                    if(ac->fullStopThresholdDir < ThresholdDir::Both) ac->fullStopThresholdDir = static_cast<ThresholdDir>(static_cast<int>(ac->fullStopThresholdDir)+1);
                    else ac->fullStopThresholdDir = ThresholdDir::None;
                }
                if(ac->useHoldDelay){
                    if(ac->fullStopThresholdDir == ThresholdDir::None || ac->fullStopThresholdDir == ThresholdDir::Both){
                        ImGui::BeginDisabled();
                    }
                    uiImLabel(_("Hold Delay (ms):"));
                    uiImPush(0);
                    uiImDrag(
                        ac->defaultThresholds || ac->fullStopThresholdDir == ThresholdDir::None || ac->fullStopThresholdDir == ThresholdDir::Both ? 
                            default_hold_val : ac->fullStopThresholdHoldDelay, 0, std::numeric_limits<int>::max());
                    uiImPop();
                    if(ac->fullStopThresholdDir == ThresholdDir::None || ac->fullStopThresholdDir == ThresholdDir::Both){
                        ImGui::EndDisabled();
                    }
                }
                uiImUnindent();
            uiImPop();
            if(ac->defaultThresholds){
                ImGui::EndDisabled();
            }

        }
        uiImUnindent();
    }

    void eventOptions(const std::shared_ptr<AnimationControl>& ac){
        uiImIndent();
            ImGui::BeginTable(__("Event"), 3, ImGuiTableFlags_SizingStretchProp);

            ImGui::TableNextColumn();
            uiImLabel(_("Play on"));
            ImGui::TableNextColumn();
            uiImLabel(_("Stop on"));
            ImGui::TableNextColumn();
            uiImLabel(_("Full stop on"));
            ImGui::TableNextColumn();

            uiImIndent();
            uiImPush(0);
            {
                bool load = hasEvent(ac->playEvent,TriggerEvent::Load);
                bool idle = hasEvent(ac->playEvent,TriggerEvent::Idle);
                bool sleep = hasEvent(ac->playEvent,TriggerEvent::Sleep);
                if(uiImCheckbox(__("Load"), load)){
                    setEvent(ac->playEvent,TriggerEvent::Load,load);
                    setEvent(ac->stopEvent,TriggerEvent::Load,false);
                    setEvent(ac->fullStopEvent,TriggerEvent::Load,false);
                    ac->triggerEvent();
                }
                if(uiImCheckbox(__("Idle"), idle)){
                    setEvent(ac->playEvent,TriggerEvent::Idle,idle);
                    if(hasEvent(ac->playEvent,TriggerEvent::Idle)){
                        setEvent(ac->stopEvent,TriggerEvent::Idle,false);
                        setEvent(ac->fullStopEvent,TriggerEvent::Idle,false);
                    }
                    ac->triggerEvent();
                }
                if(uiImCheckbox(__("Sleep"), sleep)){
                    setEvent(ac->playEvent,TriggerEvent::Sleep,sleep);
                    if(hasEvent(ac->playEvent,TriggerEvent::Sleep)){
                        setEvent(ac->stopEvent,TriggerEvent::Sleep,false);
                        setEvent(ac->fullStopEvent,TriggerEvent::Sleep,false);
                    }
                    ac->triggerEvent();
                }
            }
            uiImPop();
            uiImUnindent();

            ImGui::TableNextColumn();

            uiImIndent();
            uiImPush(1);
            {
                bool idle = hasEvent(ac->stopEvent,TriggerEvent::Idle);
                bool sleep = hasEvent(ac->stopEvent,TriggerEvent::Sleep);
                uiImDummy(vec2(0, 20));
                if(hasEvent(ac->playEvent,TriggerEvent::Idle)) ImGui::BeginDisabled();
                if(uiImCheckbox(__("Idle"), idle)){
                    setEvent(ac->stopEvent,TriggerEvent::Idle,idle);
                    ac->triggerEvent();
                }
                if(hasEvent(ac->playEvent,TriggerEvent::Idle)) ImGui::EndDisabled();
                if(hasEvent(ac->playEvent,TriggerEvent::Sleep)) ImGui::BeginDisabled();
                if(uiImCheckbox(__("Sleep"), sleep)){
                    setEvent(ac->stopEvent,TriggerEvent::Sleep,sleep);
                    ac->triggerEvent();
                } 
                if(hasEvent(ac->playEvent,TriggerEvent::Sleep)) ImGui::EndDisabled();
            }
            uiImPop();
            uiImUnindent();

            ImGui::TableNextColumn();

            uiImIndent();
            uiImPush(2);
            {
                bool idle = hasEvent(ac->fullStopEvent,TriggerEvent::Idle);
                bool sleep = hasEvent(ac->fullStopEvent,TriggerEvent::Sleep);
                uiImDummy(vec2(0, 20));
                if(hasEvent(ac->playEvent,TriggerEvent::Idle)) ImGui::BeginDisabled();
                if(uiImCheckbox(__("Idle"), idle)){
                    setEvent(ac->fullStopEvent,TriggerEvent::Idle,idle);
                    ac->triggerEvent();
                }
                if(hasEvent(ac->playEvent,TriggerEvent::Idle)) ImGui::EndDisabled();
                if(hasEvent(ac->playEvent,TriggerEvent::Sleep)) ImGui::BeginDisabled();
                if(uiImCheckbox(__("Sleep"), sleep)){
                    setEvent(ac->fullStopEvent,TriggerEvent::Sleep,sleep);
                    ac->triggerEvent();
                }
                if(hasEvent(ac->playEvent,TriggerEvent::Sleep)) ImGui::EndDisabled();
            }
            uiImPop();
            uiImUnindent();

            ImGui::EndTable();
        uiImUnindent();
        
    }
protected:

    void onUpdate() {
        auto item = insScene()->selectedSceneItem();
        if (item) {

            if (uiImButton(__("\ue5d5"))) { //Refresh
                insScene()->space->refresh();
                refresh(item->animations);
            }
            uiImTooltip(_("Refresh"));

            uiImSameLine(0, 4);

            if (uiImButton(__("\ue161"))) { //Save
                try {
                    item->saveAnimations();
                } catch (const std::exception& ex) {
                    uiImDialog(__("Error"), ex.what());
                }
            }
            uiImTooltip(_("Save to File"));
            
            for(auto& ac:item->animations) {
                uiImPush(&ac);
                auto anim = ac->anim;
                if (uiImHeader(ac->name.c_str(), true)) {

                    if (uiImButton("")) {
                        anim->stop(ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift));
                    }
                    uiImSameLine(0, 0);
                    
                    if (uiImButton(anim && !(!anim->playing() || anim->paused()) ? "" : "")) {
                        if (!anim->playing() || anim->paused()) anim->play(ac->loop);
                        else anim->pause();
                    }
                    uiImSameLine(0, 0);
                    uiImProgress((static_cast<float>(anim->frame())) / anim->frames(), vec2(-std::numeric_limits<float>::min(), 0), "");

                    uiImCheckbox(__("Loop"), ac->loop);

                    uiImDummy(vec2(0, 12));

                    uiImLabel(_("Trigger"));
                    if (uiImBeginComboBox("ACType", triggerTypeString(ac->type))) {
                        if (uiImSelectable(triggerTypeString(TriggerType::None))) {
                            ac->type = TriggerType::None;
                        }
                        if (uiImSelectable(triggerTypeString(TriggerType::Tracking))) {
                            ac->type = TriggerType::Tracking;
                        }
                        if (uiImSelectable(triggerTypeString(TriggerType::Event))) {
                            ac->type = TriggerType::Event;
                        }
                        uiImEndComboBox();
                    }

                    uiImDummy(vec2(0, 8));

                    switch(ac->type) {
                        case TriggerType::Tracking:
                            trackingOptions(ac);
                            break;
                        case TriggerType::Event:
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
    AnimationsPanel() : Panel("Animations",_("Animations"),true) {}
};
std::shared_ptr<Panel> createAnimationsPanel(){return std::make_shared<AnimationsPanel>();}
}
