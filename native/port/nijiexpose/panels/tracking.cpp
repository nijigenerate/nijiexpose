// Direct translation of dlang_archive/source/nijiexpose/panels/tracking.d.
#include "factory.hpp"
#include "tracking_state.hpp"
#include "nijiexpose/scene/package.hpp"
#include "nijiui/widgets/input.hpp"
#include "nijiui/widgets/category.hpp"
#include "nijiui/widgets/dialog.hpp"
#include "dcompat/case.hpp"
#include "i18n/tr.hpp"
#include <cassert>
#include <cstdio>
namespace nijiexpose::panels {
using namespace nijiui;using namespace tracking;using namespace scene;
using i18n::_;using i18n::__;
namespace {struct TrackingSource{bool isBone;std::string name;float dummyWidth=0;};
float axis(const Vec2& value,int index){if(index==0)return value.x;if(index==1)return value.y;throw std::out_of_range("D parameter axis");}
float mapAxis(const Parameter& param,int index,float value){int a=index==0?0:1;float range=axis(param.max,a)-axis(param.min,a);float tmp=value-axis(param.min,a);return std::clamp(tmp/range,0.f,1.f);}
std::string outputText(float value){char buf[64];std::snprintf(buf,sizeof(buf),"%g",static_cast<double>(value));auto text=_("Output (%s)");auto pos=text.find("%s");if(pos!=std::string::npos)text.replace(pos,2,buf);return text;}
}
class TrackingPanel : public Panel {
private:
    std::vector<TrackingSource> sources;
    int outVal=0;

    struct BindingCardState {
        ImDrawList* drawList=nullptr;
        ImVec2 startPos;
        ImVec2 headerBottom;
        float width;
        bool open=false;
    };

    std::vector<BindingCardState> bindingCardStack;
    std::vector<BindingCardState> bindingGroupStack;

    std::string bindingCardTitle(CompoundTrackingBinding::BindingMap item) {
        switch (item.type()) {
        case BindingType::RatioBinding:
            if (auto ratio = std::dynamic_pointer_cast<RatioTrackingBinding>(item.delegated)) {
                if (!ratio->sourceDisplayName.empty()) return ratio->sourceDisplayName;
            }
            return _("Ratio Binding");
        case BindingType::ExpressionBinding:
            return _("Expression Binding");
        case BindingType::EventBinding:
            return _("Event Binding");
        case BindingType::CompoundBinding:
            return _("Compound Binding");
        case BindingType::External:
            return _("External Binding");
        }
    }

    bool beginBindingCard(std::string title) {
        auto drawList = ImGui::GetWindowDrawList();
        drawList->ChannelsSplit(2);
        drawList->ChannelsSetCurrent(1);

        ImVec2 startPos;
        startPos = ImGui::GetCursorScreenPos();
        float width = uiImAvailableSpace().x;
        bool open = uiImHeader(title.c_str(), true);

        ImVec2 headerBottom;
        headerBottom = ImGui::GetCursorScreenPos();

        if (open) {
            uiImIndent();
            uiImDummy(vec2(0, 2));
        }

        bindingCardStack.push_back(BindingCardState{drawList, startPos, headerBottom, width, open});
        return open;
    }

    void endBindingCard() {
        if (bindingCardStack.empty()) return;

        auto state = bindingCardStack.back();
        bindingCardStack.pop_back();

        if (state.open) {
            uiImDummy(vec2(0, 2));
            uiImUnindent();
        }

        ImVec2 endPos;
        endPos = ImGui::GetCursorScreenPos();

        auto& style = ImGui::GetStyle();
        auto drawList = state.drawList;
        auto frameBg = style.Colors[ImGuiCol_FrameBg];
        auto border = ImVec4(0.0f, 0.0f, 0.0f, 0.10f);
        auto shadow = ImVec4(0.0f, 0.0f, 0.0f, 0.035f);

        drawList->ChannelsSetCurrent(0);
        drawList->AddRectFilled(state.startPos,
            ImVec2(state.startPos.x + state.width, endPos.y - 2.0f),
            ImGui::GetColorU32(ImVec4(frameBg.x, frameBg.y, frameBg.z, 0.55f)),
            8.0f
        );
        drawList->AddRect(state.startPos,
            ImVec2(state.startPos.x + state.width, endPos.y - 2.0f),
            ImGui::GetColorU32(border),
            8.0f
        );
        if (state.open) {
            drawList->AddLine(ImVec2(state.startPos.x + 8.0f, state.headerBottom.y),
                ImVec2(state.startPos.x + state.width - 8.0f, state.headerBottom.y),
                ImGui::GetColorU32(ImVec4(border.x, border.y, border.z, 0.16f)),
                1.0f
            );
        }
        drawList->AddRectFilled(ImVec2(state.startPos.x, endPos.y - 3.0f),
            ImVec2(state.startPos.x + state.width, endPos.y - 1.0f),
            ImGui::GetColorU32(shadow),
            8.0f
        );
        drawList->ChannelsSetCurrent(1);
        drawList->ChannelsMerge();

        uiImDummy(vec2(0, 8));
    }

    bool beginBindingGroup(std::string title) {
        auto drawList = ImGui::GetWindowDrawList();
        ImVec2 startPos;
        startPos = ImGui::GetCursorScreenPos();
        float width = uiImAvailableSpace().x;
        bool open = uiImHeader(title.c_str(), true);

        ImVec2 headerBottom;
        headerBottom = ImGui::GetCursorScreenPos();

        if (open) {
            uiImIndent();
            uiImDummy(vec2(0, 2));
        }

        bindingGroupStack.push_back(BindingCardState{drawList, startPos, headerBottom, width, open});
        return open;
    }

    void endBindingGroup() {
        if (bindingGroupStack.empty()) return;

        auto state = bindingGroupStack.back();
        bindingGroupStack.pop_back();

        if (state.open) {
            uiImDummy(vec2(0, 2));
            uiImUnindent();
        }

        ImVec2 endPos;
        endPos = ImGui::GetCursorScreenPos();

        if (state.open) {
            auto drawList = state.drawList;
            auto border = ImVec4(0.0f, 0.0f, 0.0f, 0.10f);
            auto shadow = ImVec4(0.0f, 0.0f, 0.0f, 0.035f);
            drawList->AddRect(ImVec2(state.startPos.x, state.headerBottom.y + 2.0f),
                ImVec2(state.startPos.x + state.width, endPos.y - 2.0f),
                ImGui::GetColorU32(border),
                8.0f
            );
            drawList->AddRectFilled(ImVec2(state.startPos.x, endPos.y - 3.0f),
                ImVec2(state.startPos.x + state.width, endPos.y - 1.0f),
                ImGui::GetColorU32(shadow),
                8.0f
            );
        }

        uiImDummy(vec2(0, 8));
    }

    // Refreshes the list of tracking sources
    void refresh(const std::vector<std::shared_ptr<TrackingBinding>>& trackingBindings) {
        auto blendshapes = insScene()->space->getAllBlendshapeNames();
        auto bones = insScene()->space->getAllBoneNames();
        ImVec2 size;
        
        sources.resize(blendshapes.size() + bones.size());
        indexableSourceNames.resize(sources.size());
        minValues.resize(sources.size());
        maxValues.resize(sources.size());

        for(size_t i=0;i<blendshapes.size();++i){auto& blendshape=blendshapes[i];
            sources[i] = TrackingSource(
                false,
                blendshape
            );

            size = ImGui::CalcTextSize(blendshape.c_str());
            trackingSourceNameWidth = std::max(trackingSourceNameWidth, size.x);
            indexableSourceNames[i] = dcompat::toLower(blendshape);
            minValues[i] = 0;
            maxValues[i] = 1;
        }

        for(size_t i=0;i<bones.size();++i){auto& bone=bones[i];
            sources[blendshapes.size()+i] = TrackingSource(
                true,
                bone
            );

            size = ImGui::CalcTextSize(bone.c_str());
            trackingSourceNameWidth = std::max(trackingSourceNameWidth, size.x);
            indexableSourceNames[blendshapes.size()+i] = dcompat::toLower(bone);
            minValues[i] = -1;
            maxValues[i] = 1;
        }

        // Add any bindings unnacounted for which are stored in the model.
        for(auto& bind_:trackingBindings) {

            if (auto bind = std::dynamic_pointer_cast<RatioTrackingBinding>(bind_->delegated)) {
                
                // Skip non-existent sources
                if (bind->sourceName.empty()) continue;

                TrackingSource src = TrackingSource(
                    bind->sourceType != SourceType::Blendshape,
                    bind->sourceName
                );

                size = ImGui::CalcTextSize(bind->sourceName.c_str());
                trackingSourceNameWidth = std::max(trackingSourceNameWidth, size.x);

                // Skip anything we already know
                if(std::any_of(sources.begin(),sources.end(),[&](auto& xsrc){return xsrc.isBone==src.isBone && xsrc.name==src.name;}))continue;

                sources.push_back(src);
                indexableSourceNames.push_back(dcompat::toLower(src.name));
                minValues.push_back(0);
                maxValues.push_back(1);
            }
        }
        for(auto& source:sources) {
            size = ImGui::CalcTextSize(source.name.c_str());
            source.dummyWidth = trackingSourceNameWidth - size.x;
        }
    }

    
    // Settings popup for binding types
    template<class T> bool settingsPopup(T binding) {
        bool changed = false;
        if (uiImBeginPopup("BINDING_SETTINGS")) {
            if (uiImBeginMenu(__("Type"))) {

                if (uiImMenuItem(__("Ratio Binding"))) {
                    if (binding->type() != BindingType::RatioBinding)
                        changed = true;
                    binding->type(BindingType::RatioBinding);
                }

                if (uiImMenuItem(__("Expression Binding"))) {
                    if (binding->type() != BindingType::ExpressionBinding)
                        changed = true;
                    binding->type(BindingType::ExpressionBinding);
                    changed = true;
                }

                if (uiImMenuItem(__("Event Binding"))) {
                    if (binding->type() != BindingType::EventBinding)
                        changed = true;
                    binding->type(BindingType::EventBinding);
                    changed = true;
                }

                if (uiImMenuItem(__("Compound Binding"))) {
                    if (binding->type() != BindingType::CompoundBinding)
                        changed = true;
                    binding->type(BindingType::CompoundBinding);
                    changed = true;
                }

                uiImEndMenu();
            }
            uiImEndPopup();
        }

        if (uiImButton("\ue5d2")) {
            uiImOpenPopup("BINDING_SETTINGS");
        }

        return changed;
    }

    // Configuration panel for expression bindings
    void exprBinding(size_t i, const std::shared_ptr<ITrackingBinding>& binding) {
        auto eBinding = std::dynamic_pointer_cast<ExpressionTrackingBinding>(binding);
        if (eBinding->expr) {
            std::string buf = eBinding->expr->expression();
            if(eBinding->binding() && eBinding->binding()->delegated == eBinding)
                if (settingsPopup(eBinding->binding()))
                    return;
            
            uiImLabel(_("Dampen"));
            ImGui::SliderInt("", &eBinding->dampenLevel, 0, 10);

            if (uiImInputText("###EXPRESSION", buf)) {
                eBinding->expr->expression(std::string(dcompat::fromStringz(buf)));
            }

            if (!eBinding->expr->lastError().empty()) {
                uiImLabelColored("\ue000", vec4(1, 0.4, 0.4, 1));
                uiImTooltip(eBinding->expr->lastError());
                uiImSameLine();
            }
            if (eBinding->outVal < 0 || eBinding->outVal > 1) {
                uiImLabelColored("\ue002", vec4(0.5, 0.5, 0.2, 1));
                uiImTooltip(_("Value out of range, clamped to 0..1 range."));
                uiImSameLine();
            }
            uiImLabel(outputText(eBinding->outVal));
            uiImIndent();
                uiImProgress(eBinding->outVal);
            uiImUnindent();
        }
    }

    // Configuration panel for ratio bindings
    void ratioBinding(size_t i, const std::shared_ptr<ITrackingBinding>& binding) {
        auto rBinding = std::dynamic_pointer_cast<RatioTrackingBinding>(binding);
        if (!rBinding) return;

        if (rBinding->binding && rBinding->binding->delegated == rBinding) {
            if (settingsPopup(rBinding->binding))
                return;
            ImGui::SameLine();
        }

        bool hasTrackingSrc = !rBinding->sourceName.empty();

        if (uiImBeginComboBox("SELECTION_COMBO", hasTrackingSrc ? rBinding->sourceDisplayName.c_str() : __("Not tracked"))) {
            std::string filter = trackingFilter;
            if (uiImInputText("###FILTER", uiImAvailableSpace().x, filter)) {
                trackingFilter = std::string(dcompat::fromStringz(dcompat::toLower(filter)));
            }

            uiImDummy(vec2(0, 8));
               
            for(size_t ix=0;ix<sources.size();++ix){auto& source=sources[ix];
                if (!trackingFilter.empty() && indexableSourceNames[ix].find(trackingFilter)==std::string::npos) continue;

                bool selected = rBinding->sourceName == source.name;
                bool nameValid = !source.name.empty();
                if (source.isBone) {
                    if (uiImBeginMenu(source.name.c_str())) {
                        if (uiImMenuItem(__("X"))) {
                            rBinding->sourceName = source.name;
                            rBinding->sourceType = SourceType::BonePosX;
                            rBinding->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        if (uiImMenuItem(__("Y"))) {
                            rBinding->sourceName = source.name;
                            rBinding->sourceType = SourceType::BonePosY;
                            rBinding->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        if (uiImMenuItem(__("Z"))) {
                            rBinding->sourceName = source.name;
                            rBinding->sourceType = SourceType::BonePosZ;
                            rBinding->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        if (uiImMenuItem(__("Roll"))) {
                            rBinding->sourceName = source.name;
                            rBinding->sourceType = SourceType::BoneRotRoll;
                            rBinding->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        if (uiImMenuItem(__("Pitch"))) {
                            rBinding->sourceName = source.name;
                            rBinding->sourceType = SourceType::BoneRotPitch;
                            rBinding->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        if (uiImMenuItem(__("Yaw"))) {
                            rBinding->sourceName = source.name;
                            rBinding->sourceType = SourceType::BoneRotYaw;
                            rBinding->createSourceDisplayName();
                            trackingFilter.clear();
                        }
                        uiImEndMenu();
                    }
                } else {
                    if (ImGui::Selectable(nameValid? source.name.c_str(): "###NoName", selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
                        trackingFilter.clear();
                        rBinding->sourceType = SourceType::Blendshape;
                        rBinding->sourceName = source.name;
                        rBinding->createSourceDisplayName();
                    }
                    uiImSameLine();
                    uiImDummy(vec2(source.dummyWidth + 1, 1));
                    uiImSameLine();
                    auto zone = insScene()->space->currentZone();
                    float value = zone ? zone->getBlendshapeFor(source.name) : 0.0f;
                    if (value < minValues[ix]) minValues[ix] = value;
                    if (value > maxValues[ix]) maxValues[ix] = value;
                    ImGui::ProgressBar((value - minValues[ix]) / (maxValues[ix] - minValues[ix]), ImVec2(0, 10), "");
                }
            }
            uiImEndComboBox();
        }
        if (hasTrackingSrc)
            ImGui::SameLine();
        if (hasTrackingSrc && uiImButton(__("\ue5cd"))) {
            rBinding->sourceName.clear();
        }

        if (hasTrackingSrc) {
            uiImCheckbox(__("Inverse"), rBinding->inverse);

            uiImLabel(_("Dampen"));
            ImGui::SliderInt("", &rBinding->dampenLevel, 0, 10);

            uiImLabel(_("Tracking In"));
            uiImPush(0);
                uiImIndent();
                    ImGui::SetNextItemWidth (96);
                    switch(rBinding->sourceType) {
                        case SourceType::Blendshape:
                            // TODO: Make all blendshapes in facetrack-d 0->1
                            uiImRange(rBinding->inRange.x, rBinding->inRange.y, -1, 1);
                            break;

                        case SourceType::BonePosX:
                        case SourceType::BonePosY:
                        case SourceType::BonePosZ:
                            uiImRange(rBinding->inRange.x, rBinding->inRange.y, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
                            break;

                        case SourceType::BoneRotPitch:
                        case SourceType::BoneRotRoll:
                        case SourceType::BoneRotYaw:
                            uiImRange(rBinding->inRange.x, rBinding->inRange.y, -180, 180);
                            break;
                            
                        default: assert(0);
                    }
                    ImGui::SameLine();
                    uiImProgress(rBinding->inVal, vec2(-std::numeric_limits<float>::min(), 0), "");
                uiImUnindent();
            uiImPop();
            
            uiImLabel(_("Tracking Out"));
            uiImPush(1);
                uiImIndent();
                    ImGui::SetNextItemWidth (96);
                    uiImRange(rBinding->outRange.x, rBinding->outRange.y, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
                    ImGui::SameLine();
                    uiImProgress(mapAxis(*rBinding->binding->param,rBinding->binding->axis,rBinding->outVal), vec2(-std::numeric_limits<float>::min(), 0), "");
                uiImUnindent();
            uiImPop();
        }
    }

    // Configuration panel for event bindings
    void eventBinding(size_t i, const std::shared_ptr<ITrackingBinding>& binding) {
        auto eBinding = std::dynamic_pointer_cast<EventTrackingBinding>(binding);
        if (eBinding) {
            if (eBinding->binding && eBinding->binding->delegated == eBinding)
                if (settingsPopup(eBinding->binding))
                    return;

            uiImLabel(_("Dampen"));
            ImGui::SliderInt("", &eBinding->dampenLevel, 0, 10);

            int indexToRemove = -1;
            for(size_t idx=0;idx<eBinding->valueMap.size();++idx){auto item=eBinding->valueMap[idx];
                uiImPush(static_cast<int>(idx) + 1);
                std::string idHold = item.id;
                if (uiImInputText("###EVENTID", 64, idHold)) {
                    eBinding->valueMap[idx].id = dcompat::toUpper(dcompat::fromStringz(idHold));
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(128);
                ImGui::DragFloat("", &(eBinding->valueMap[idx].value), 0.01, axis(eBinding->binding->param->min,eBinding->binding->axis), axis(eBinding->binding->param->max,eBinding->binding->axis));
                ImGui::SameLine();
                if (uiImButton(__("\ue5cd"))) {
                    indexToRemove = static_cast<int>(idx);
                }
                uiImPop();
            }
            if (indexToRemove >= 0) {
                eBinding->valueMap.erase(eBinding->valueMap.begin()+indexToRemove);
            }
            {
                if (uiImButton(__("\ue145"))) {
                    eBinding->valueMap.push_back(EventTrackingBinding::EventMap{SourceType::KeyPress,"",0});
                }
            }

            if (eBinding->outVal < 0 || eBinding->outVal > 1) {
                uiImLabelColored("\ue002", vec4(0.5, 0.5, 0.2, 1));
                uiImTooltip(_("Value out of range, clamped to 0..1 range."));
                uiImSameLine();
            }
            uiImLabel(outputText(eBinding->outVal));
            uiImIndent();
                uiImProgress(eBinding->outVal);            
            uiImUnindent();
        }
    }

    // Configuration panel for event bindings
    void compoundBinding(size_t i, const std::shared_ptr<ITrackingBinding>& binding) {
        auto cBinding = std::dynamic_pointer_cast<CompoundTrackingBinding>(binding);
        if (cBinding) {
            if (cBinding->binding() && cBinding->binding()->delegated == cBinding) {
                if (settingsPopup(cBinding->binding()))
                    return;
                ImGui::SameLine();
            }
            std::map<CompoundTrackingBinding::Method,const char*> methodMap{
                {CompoundTrackingBinding::Method::WeightedSum, __("Weighted Sum")},
                {CompoundTrackingBinding::Method::WeightedMul, __("Weighted Multiply")},
                {CompoundTrackingBinding::Method::Ordered, __("Ordered")},
            };
            if (uiImBeginComboBox("COMPOUND_COMBO", methodMap[cBinding->method])) {
                for(auto& [key,value]:methodMap) {
                    if (uiImMenuItem(value, nullptr, cBinding->method == key, true)) {
                        cBinding->method = key;
                    }

                }
                uiImEndComboBox();
            }

//            uiImLabel(_("Dampen"));
//            ImGui::SliderInt("", &cBinding->binding()->dampenLevel, 0, 10);

            int indexToRemove = -1;
            for(size_t idx=0;idx<cBinding->bindingMap.size();++idx){auto item=cBinding->bindingMap[idx];
                uiImPush(static_cast<int>(idx) + 1);
                if (uiImBeginCategory(bindingCardTitle(item).c_str())) {
                        settingsPopup(&cBinding->bindingMap[idx]);
                        ImGui::SameLine();
                        float weight = item.weight;
                        if (ImGui::DragFloat("###1", &weight, 0, 1)) {
                            cBinding->bindingMap[idx].weight = weight;
                        }
                        ImGui::SameLine();
                        if (uiImButton(__("\ue5cd"))) {
                            indexToRemove = static_cast<int>(idx);
                        }
                        switch (item.type()) {
                            case BindingType::RatioBinding:
                                ratioBinding(idx, item.delegated);
                                break;
                            case BindingType::ExpressionBinding:
                                exprBinding(idx, item.delegated);
                                break;
                            case BindingType::EventBinding:
                                eventBinding(idx, item.delegated);
                                break;
                            case BindingType::CompoundBinding:
                                compoundBinding(idx, item.delegated);
                                break;
                            default:
                                break;
                        }
                }
                uiImEndCategory();
                uiImPop();
            }
            if (indexToRemove >= 0) {
                cBinding->bindingMap.erase(cBinding->bindingMap.begin()+indexToRemove);
            }
            {
                if (uiImButton(__("\ue145"))) {
                    cBinding->bindingMap.emplace_back(cBinding.get(),1,BindingType::RatioBinding);
                }
            }

            if (cBinding->outVal < 0 || cBinding->outVal > 1) {
                uiImLabelColored("\ue002", vec4(0.8, 0.7, 0.2, 1));
                uiImTooltip(_("Value out of range, clamped to 0..1 range."));
                uiImSameLine();
            }
            uiImLabel(outputText(cBinding->outVal));
            uiImIndent();
                uiImProgress(mapAxis(*cBinding->binding()->param,cBinding->binding()->axis,cBinding->outVal), vec2(-std::numeric_limits<float>::min(), 0), "");            
            uiImUnindent();
        }
    }

protected:

    void onUpdate() {
        auto item = insScene()->selectedSceneItem();
        if (item) {
            if (indexableSourceNames.empty() || uiImButton(__("\ue5d5"))) {//Refresh
                insScene()->space->refresh();
                refresh(item->bindings);
            }
            uiImTooltip(_("Refresh"));

            uiImSameLine(0, 4);

            if (uiImButton(__("\ue161"))) { //Save
                try {
                item->saveBindings();
                } catch (const std::exception& ex) {
                    uiImDialog(__("Error"), ex.what());
                }
            }
            uiImTooltip(_("Save to File"));

            for(size_t i=0;i<item->bindings.size();++i){auto& binding=item->bindings[i];
                uiImPush(&binding);
                    if (uiImBeginCategory(binding->name.c_str())) {
                        switch(binding->type()) {

                            case BindingType::RatioBinding:
                                ratioBinding(i, binding->delegated);
                                break;

                            case BindingType::ExpressionBinding:
                                exprBinding(i, binding->delegated);
                                break;

                            case BindingType::EventBinding:
                                eventBinding(i, binding->delegated);
                                break;

                            case BindingType::CompoundBinding:
                                compoundBinding(i, binding->delegated);
                                break;

                            // External bindings
                            default: 
                                settingsPopup(binding);
                                uiImLabel(_("No settings available."));
                                break;
                        }
                    }
                    uiImEndCategory();
                uiImPop();
            }
        } else uiImLabel(_("No puppet selected"));
    }

public:
    TrackingPanel() : Panel("Tracking",_("Tracking"),true) {}
};
std::shared_ptr<Panel> createTrackingPanel(){return std::make_shared<TrackingPanel>();}
}
