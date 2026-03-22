/*
    Copyright © 2022, Inochi2D Project
    Copyright © 2024, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.panels.tracking;
import nijiexpose.tracking.expr;
import nijiui.panel;
import i18n;
import nijiexpose.scene;
import nijiexpose.tracking;
import nijiui;
import nijiui.widgets;
import nijiexpose.log;
import bindbc.imgui;
import inmath;
import std.string;
import std.uni;
import std.array;
import std.algorithm.searching;
import std.algorithm.mutation;

private {
    string trackingFilter;
    const(char)*[] paramNames;
    string[] indexableSourceNames;
    float[] minValues;
    float[] maxValues;

    float trackingSourceNameWidth = 0;

    struct TrackingSource {
        bool isBone;
        string name;
        const(char)* cName;
        float dummyWidth;
    }
}

void neTrackingPanelReset() {
    indexableSourceNames.length = 0;
    minValues.length = 0;
    maxValues.length = 0;
}

// Refreshes the tracking bindings listed in the tracking panel (headers)
void insTrackingPanelRefresh() {
    trackingFilter = "";
    paramNames = null;    
    if (insScene.selectedSceneItem()) {
        foreach(ref TrackingBinding binding; insScene.selectedSceneItem().bindings) {
            paramNames ~= binding.param.name.toStringz;
        }
    }
}

class TrackingPanel : Panel {
private:
    TrackingSource[] sources;
    int outVal;

    struct BindingCardState {
        ImDrawList* drawList;
        ImVec2 startPos;
        ImVec2 headerBottom;
        float width;
        bool open;
    }

    BindingCardState[] bindingCardStack;
    BindingCardState[] bindingGroupStack;

    string bindingCardTitle(CompoundTrackingBinding.BindingMap item) {
        final switch (item.type) {
        case BindingType.RatioBinding:
            if (auto ratio = cast(RatioTrackingBinding)item.delegated) {
                if (ratio.sourceDisplayName.length > 0) return ratio.sourceDisplayName;
            }
            return _("Ratio Binding");
        case BindingType.ExpressionBinding:
            return _("Expression Binding");
        case BindingType.EventBinding:
            return _("Event Binding");
        case BindingType.CompoundBinding:
            return _("Compound Binding");
        case BindingType.External:
            return _("External Binding");
        }
    }

    bool beginBindingCard(string title) {
        auto drawList = igGetWindowDrawList();
        ImDrawList_ChannelsSplit(drawList, 2);
        ImDrawList_ChannelsSetCurrent(drawList, 1);

        ImVec2 startPos;
        igGetCursorScreenPos(&startPos);
        float width = uiImAvailableSpace().x;
        bool open = uiImHeader(title.toStringz, true);

        ImVec2 headerBottom;
        igGetCursorScreenPos(&headerBottom);

        if (open) {
            uiImIndent();
            uiImDummy(vec2(0, 2));
        }

        bindingCardStack ~= BindingCardState(drawList, startPos, headerBottom, width, open);
        return open;
    }

    void endBindingCard() {
        if (bindingCardStack.length == 0) return;

        auto state = bindingCardStack[$ - 1];
        bindingCardStack.length -= 1;

        if (state.open) {
            uiImDummy(vec2(0, 2));
            uiImUnindent();
        }

        ImVec2 endPos;
        igGetCursorScreenPos(&endPos);

        auto style = igGetStyle();
        auto drawList = state.drawList;
        auto frameBg = style.Colors[ImGuiCol.FrameBg];
        auto border = ImVec4(0.0f, 0.0f, 0.0f, 0.10f);
        auto shadow = ImVec4(0.0f, 0.0f, 0.0f, 0.035f);

        ImDrawList_ChannelsSetCurrent(drawList, 0);
        ImDrawList_AddRectFilled(
            drawList,
            state.startPos,
            ImVec2(state.startPos.x + state.width, endPos.y - 2.0f),
            igGetColorU32(ImVec4(frameBg.x, frameBg.y, frameBg.z, 0.55f)),
            8.0f
        );
        ImDrawList_AddRect(
            drawList,
            state.startPos,
            ImVec2(state.startPos.x + state.width, endPos.y - 2.0f),
            igGetColorU32(border),
            8.0f
        );
        if (state.open) {
            ImDrawList_AddLine(
                drawList,
                ImVec2(state.startPos.x + 8.0f, state.headerBottom.y),
                ImVec2(state.startPos.x + state.width - 8.0f, state.headerBottom.y),
                igGetColorU32(ImVec4(border.x, border.y, border.z, 0.16f)),
                1.0f
            );
        }
        ImDrawList_AddRectFilled(
            drawList,
            ImVec2(state.startPos.x, endPos.y - 3.0f),
            ImVec2(state.startPos.x + state.width, endPos.y - 1.0f),
            igGetColorU32(shadow),
            8.0f
        );
        ImDrawList_ChannelsSetCurrent(drawList, 1);
        ImDrawList_ChannelsMerge(drawList);

        uiImDummy(vec2(0, 8));
    }

    bool beginBindingGroup(string title) {
        auto drawList = igGetWindowDrawList();
        ImVec2 startPos;
        igGetCursorScreenPos(&startPos);
        float width = uiImAvailableSpace().x;
        bool open = uiImHeader(title.toStringz, true);

        ImVec2 headerBottom;
        igGetCursorScreenPos(&headerBottom);

        if (open) {
            uiImIndent();
            uiImDummy(vec2(0, 2));
        }

        bindingGroupStack ~= BindingCardState(drawList, startPos, headerBottom, width, open);
        return open;
    }

    void endBindingGroup() {
        if (bindingGroupStack.length == 0) return;

        auto state = bindingGroupStack[$ - 1];
        bindingGroupStack.length -= 1;

        if (state.open) {
            uiImDummy(vec2(0, 2));
            uiImUnindent();
        }

        ImVec2 endPos;
        igGetCursorScreenPos(&endPos);

        if (state.open) {
            auto drawList = state.drawList;
            auto border = ImVec4(0.0f, 0.0f, 0.0f, 0.10f);
            auto shadow = ImVec4(0.0f, 0.0f, 0.0f, 0.035f);
            ImDrawList_AddRect(
                drawList,
                ImVec2(state.startPos.x, state.headerBottom.y + 2.0f),
                ImVec2(state.startPos.x + state.width, endPos.y - 2.0f),
                igGetColorU32(border),
                8.0f
            );
            ImDrawList_AddRectFilled(
                drawList,
                ImVec2(state.startPos.x, endPos.y - 3.0f),
                ImVec2(state.startPos.x + state.width, endPos.y - 1.0f),
                igGetColorU32(shadow),
                8.0f
            );
        }

        uiImDummy(vec2(0, 8));
    }

    // Refreshes the list of tracking sources
    void refresh(ref TrackingBinding[] trackingBindings) {
        auto blendshapes = insScene.space.getAllBlendshapeNames();
        auto bones = insScene.space.getAllBoneNames();
        ImVec2 size;
        
        sources.length = blendshapes.length + bones.length;
        indexableSourceNames.length = sources.length;
        minValues.length = sources.length;
        maxValues.length = sources.length;

        foreach(i, blendshape; blendshapes) {
            sources[i] = TrackingSource(
                false,
                blendshape,
                blendshape.toStringz
            );

            igCalcTextSize(&size, blendshape.toStringz);
            trackingSourceNameWidth = max(trackingSourceNameWidth, size.x);
            indexableSourceNames[i] = blendshape.toLower;
            minValues[i] = 0;
            maxValues[i] = 1;
        }

        foreach(i, bone; bones) {
            sources[blendshapes.length+i] = TrackingSource(
                true,
                bone,
                bone.toStringz
            );

            igCalcTextSize(&size, bone.toStringz);
            trackingSourceNameWidth = max(trackingSourceNameWidth, size.x);
            indexableSourceNames[blendshapes.length+i] = bone.toLower;
            minValues[i] = -1;
            maxValues[i] = 1;
        }

        // Add any bindings unnacounted for which are stored in the model.
        trkMain: foreach(bind_; trackingBindings) {

            if (auto bind = cast(RatioTrackingBinding)bind_.delegated) {
                
                // Skip non-existent sources
                if (bind.sourceName.length == 0) continue;

                TrackingSource src = TrackingSource(
                    bind.sourceType != SourceType.Blendshape,
                    bind.sourceName,
                    bind.sourceName.toStringz
                );

                igCalcTextSize(&size, bind.sourceName.toStringz);
                trackingSourceNameWidth = max(trackingSourceNameWidth, size.x);

                // Skip anything we already know
                foreach(xsrc; sources) {
                    if (xsrc.isBone == src.isBone && xsrc.name == src.name) continue trkMain;
                }

                sources ~= src;
                indexableSourceNames ~= src.name.toLower;
                minValues ~= 0;
                maxValues ~= 1;
            }
        }
        foreach (ref TrackingSource source; sources) {
            igCalcTextSize(&size, source.cName);
            source.dummyWidth = trackingSourceNameWidth - size.x;
        }
    }

    
    // Settings popup for binding types
    pragma(inline, true)
    bool settingsPopup(T)(T binding) {
        bool changed = false;
        if (uiImBeginPopup("BINDING_SETTINGS")) {
            if (uiImBeginMenu(__("Type"))) {

                if (uiImMenuItem(__("Ratio Binding"))) {
                    if (binding.type != BindingType.RatioBinding)
                        changed = true;
                    binding.type = BindingType.RatioBinding;
                }

                if (uiImMenuItem(__("Expression Binding"))) {
                    if (binding.type != BindingType.ExpressionBinding)
                        changed = true;
                    binding.type = BindingType.ExpressionBinding;
                    changed = true;
                }

                if (uiImMenuItem(__("Event Binding"))) {
                    if (binding.type != BindingType.EventBinding)
                        changed = true;
                    binding.type = BindingType.EventBinding;
                    changed = true;
                }

                if (uiImMenuItem(__("Compound Binding"))) {
                    if (binding.type != BindingType.CompoundBinding)
                        changed = true;
                    binding.type = BindingType.CompoundBinding;
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
    void exprBinding(size_t i, ITrackingBinding binding) {
        auto eBinding = cast(ExpressionTrackingBinding)binding;
        if (eBinding.expr) {
            string buf = eBinding.expr.expression.dup;
            if(eBinding.binding && eBinding.binding.delegated == eBinding)
                if (settingsPopup(eBinding.binding))
                    return;
            
            uiImLabel(_("Dampen"));
            igSliderInt("", &eBinding.dampenLevel, 0, 10);

            if (uiImInputText("###EXPRESSION", buf)) {
                eBinding.expr.expression = buf.toStringz.fromStringz;
            }

            if (eBinding.expr.lastError.length > 0) {
                uiImLabelColored("\ue000", vec4(1, 0.4, 0.4, 1));
                uiImTooltip(eBinding.expr.lastError);
                uiImSameLine();
            }
            if (eBinding.outVal < 0 || eBinding.outVal > 1) {
                uiImLabelColored("\ue002", vec4(0.5, 0.5, 0.2, 1));
                uiImTooltip(_("Value out of range, clamped to 0..1 range."));
                uiImSameLine();
            }
            uiImLabel(_("Output (%s)").format(eBinding.outVal));
            uiImIndent();
                uiImProgress(eBinding.outVal);
            uiImUnindent();
        }
    }

    // Configuration panel for ratio bindings
    void ratioBinding(size_t i, ITrackingBinding binding) {
        auto rBinding = cast(RatioTrackingBinding)binding;
        if (!rBinding) return;

        if (rBinding.binding && rBinding.binding.delegated == rBinding) {
            if (settingsPopup(rBinding.binding))
                return;
            igSameLine();
        }

        bool hasTrackingSrc = rBinding.sourceName.length > 0;

        if (uiImBeginComboBox("SELECTION_COMBO", hasTrackingSrc ? rBinding.sourceDisplayName.toStringz : __("Not tracked"))) {
            string filter = trackingFilter.dup;
            if (uiImInputText("###FILTER", uiImAvailableSpace().x, filter)) {
                trackingFilter = filter.toLower().toStringz.fromStringz;
            }

            uiImDummy(vec2(0, 8));
               
            foreach(ix, source; sources) {
                if (trackingFilter.length > 0 && !indexableSourceNames[ix].canFind(trackingFilter)) continue;

                bool selected = rBinding.sourceName == source.name;
                bool nameValid = source.name.length > 0;
                if (source.isBone) {
                    if (uiImBeginMenu(source.cName)) {
                        if (uiImMenuItem(__("X"))) {
                            rBinding.sourceName = source.name;
                            rBinding.sourceType = SourceType.BonePosX;
                            rBinding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Y"))) {
                            rBinding.sourceName = source.name;
                            rBinding.sourceType = SourceType.BonePosY;
                            rBinding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Z"))) {
                            rBinding.sourceName = source.name;
                            rBinding.sourceType = SourceType.BonePosZ;
                            rBinding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Roll"))) {
                            rBinding.sourceName = source.name;
                            rBinding.sourceType = SourceType.BoneRotRoll;
                            rBinding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Pitch"))) {
                            rBinding.sourceName = source.name;
                            rBinding.sourceType = SourceType.BoneRotPitch;
                            rBinding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Yaw"))) {
                            rBinding.sourceName = source.name;
                            rBinding.sourceType = SourceType.BoneRotYaw;
                            rBinding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        uiImEndMenu();
                    }
                } else {
                    if (igSelectable(nameValid? source.cName: "###NoName", selected, ImGuiSelectableFlags.SpanAllColumns | ImGuiSelectableFlags.AllowItemOverlap)) {
                        trackingFilter = null;
                        rBinding.sourceType = SourceType.Blendshape;
                        rBinding.sourceName = source.name;
                        rBinding.createSourceDisplayName();
                    }
                    uiImSameLine();
                    uiImDummy(vec2(source.dummyWidth + 1, 1));
                    uiImSameLine();
                    float value = insScene.space.currentZone.getBlendshapeFor(source.name);
                    if (value < minValues[ix]) minValues[ix] = value;
                    if (value > maxValues[ix]) maxValues[ix] = value;
                    igProgressBar((value - minValues[ix]) / (maxValues[ix] - minValues[ix]), ImVec2(0, 10), "");
                }
            }
            uiImEndComboBox();
        }
        if (hasTrackingSrc)
            igSameLine();
        if (hasTrackingSrc && uiImButton(__("\ue5cd"))) {
            rBinding.sourceName = null;
        }

        if (hasTrackingSrc) {
            uiImCheckbox(__("Inverse"), rBinding.inverse);

            uiImLabel(_("Dampen"));
            igSliderInt("", &rBinding.dampenLevel, 0, 10);

            uiImLabel(_("Tracking In"));
            uiImPush(0);
                uiImIndent();
                    igSetNextItemWidth (96);
                    switch(rBinding.sourceType) {
                        case SourceType.Blendshape:
                            // TODO: Make all blendshapes in facetrack-d 0->1
                            uiImRange(rBinding.inRange.x, rBinding.inRange.y, -1, 1);
                            break;

                        case SourceType.BonePosX:
                        case SourceType.BonePosY:
                        case SourceType.BonePosZ:
                            uiImRange(rBinding.inRange.x, rBinding.inRange.y, -float.max, float.max);
                            break;

                        case SourceType.BoneRotPitch:
                        case SourceType.BoneRotRoll:
                        case SourceType.BoneRotYaw:
                            uiImRange(rBinding.inRange.x, rBinding.inRange.y, -180, 180);
                            break;
                            
                        default: assert(0);
                    }
                    igSameLine();
                    uiImProgress(rBinding.inVal, vec2(-float.min_normal, 0), "");
                uiImUnindent();
            uiImPop();
            
            uiImLabel(_("Tracking Out"));
            uiImPush(1);
                uiImIndent();
                    igSetNextItemWidth (96);
                    uiImRange(rBinding.outRange.x, rBinding.outRange.y, -float.max, float.max);
                    igSameLine();
                    uiImProgress(rBinding.binding.param.mapAxis(rBinding.binding.axis, rBinding.outVal), vec2(-float.min_normal, 0), "");
                uiImUnindent();
            uiImPop();
        }
    }

    // Configuration panel for event bindings
    void eventBinding(size_t i, ITrackingBinding binding) {
        auto eBinding = cast(EventTrackingBinding)binding;
        if (eBinding) {
            if (eBinding.binding && eBinding.binding.delegated == eBinding)
                if (settingsPopup(eBinding.binding))
                    return;

            uiImLabel(_("Dampen"));
            igSliderInt("", &eBinding.dampenLevel, 0, 10);

            int indexToRemove = -1;
            foreach (idx, item; eBinding.valueMap) {
                uiImPush(cast(int)idx + 1);
                string idHold = item.id.dup;
                if (uiImInputText("###EVENTID", 64, idHold)) {
                    eBinding.valueMap[idx].id = idHold.toStringz.fromStringz.toUpper();
                }
                igSameLine();
                igSetNextItemWidth(128);
                igDragFloat("", &(eBinding.valueMap[idx].value), 0.01, eBinding.binding.param.min.vector[eBinding.binding.axis], eBinding.binding.param.max.vector[eBinding.binding.axis]);
                igSameLine();
                if (uiImButton(__("\ue5cd"))) {
                    indexToRemove = cast(int)idx;
                }
                uiImPop();
            }
            if (indexToRemove >= 0) {
                eBinding.valueMap = eBinding.valueMap.remove(indexToRemove);
            }
            {
                if (uiImButton(__("\ue145"))) {
                    eBinding.valueMap ~= EventTrackingBinding.EventMap(SourceType.KeyPress, "", 0);
                }
            }

            if (eBinding.outVal < 0 || eBinding.outVal > 1) {
                uiImLabelColored("\ue002", vec4(0.5, 0.5, 0.2, 1));
                uiImTooltip(_("Value out of range, clamped to 0..1 range."));
                uiImSameLine();
            }
            uiImLabel(_("Output (%s)").format(eBinding.outVal));
            uiImIndent();
                uiImProgress(eBinding.outVal);            
            uiImUnindent();
        }
    }

    // Configuration panel for event bindings
    void compoundBinding(size_t i, ITrackingBinding binding) {
        auto cBinding = cast(CompoundTrackingBinding)binding;
        if (cBinding) {
            if (cBinding.binding && cBinding.binding.delegated == cBinding) {
                if (settingsPopup(cBinding.binding))
                    return;
                igSameLine();
            }
            auto methodMap = [
                CompoundTrackingBinding.Method.WeightedSum: __("Weighted Sum"),
                CompoundTrackingBinding.Method.WeightedMul: __("Weighted Multiply"),
                CompoundTrackingBinding.Method.Ordered: __("Ordered"),
            ];
            if (uiImBeginComboBox("COMPOUND_COMBO", methodMap[cBinding.method])) {
                foreach (key, value; methodMap) {
                    if (uiImMenuItem(value, null, cBinding.method == key, true)) {
                        cBinding.method = key;
                    }

                }
                uiImEndComboBox();
            }

//            uiImLabel(_("Dampen"));
//            igSliderInt("", &cBinding.binding.dampenLevel, 0, 10);

            int indexToRemove = -1;
            foreach (idx, item; cBinding.bindingMap) {
                uiImPush(cast(int)idx + 1);
                if (uiImBeginCategory(bindingCardTitle(item).toStringz)) {
                        settingsPopup(&cBinding.bindingMap[idx]);
                        igSameLine();
                        float weight = item.weight;
                        if (igDragFloat("###1", &weight, 0, 1)) {
                            cBinding.bindingMap[idx].weight = weight;
                        }
                        igSameLine();
                        if (uiImButton(__("\ue5cd"))) {
                            indexToRemove = cast(int)idx;
                        }
                        switch (item.type) {
                            case BindingType.RatioBinding:
                                ratioBinding(idx, item.delegated);
                                break;
                            case BindingType.ExpressionBinding:
                                exprBinding(idx, item.delegated);
                                break;
                            case BindingType.EventBinding:
                                eventBinding(idx, item.delegated);
                                break;
                            case BindingType.CompoundBinding:
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
                cBinding.bindingMap = cBinding.bindingMap.remove(indexToRemove);
            }
            {
                if (uiImButton(__("\ue145"))) {
                    cBinding.bindingMap ~= CompoundTrackingBinding.BindingMap(cBinding, 1, BindingType.RatioBinding);
                }
            }

            if (cBinding.outVal < 0 || cBinding.outVal > 1) {
                uiImLabelColored("\ue002", vec4(0.8, 0.7, 0.2, 1));
                uiImTooltip(_("Value out of range, clamped to 0..1 range."));
                uiImSameLine();
            }
            uiImLabel(_("Output (%s)").format(cBinding.outVal));
            uiImIndent();
                uiImProgress(cBinding.binding.param.mapAxis(cBinding.binding.axis, cBinding.outVal), vec2(-float.min_normal, 0), "");            
            uiImUnindent();
        }
    }

protected:

    override 
    void onUpdate() {
        auto item = insScene.selectedSceneItem();
        if (item) {
            if (indexableSourceNames.length == 0 || uiImButton(__("\ue5d5"))) {//Refresh
                insScene.space.refresh();
                refresh(item.bindings);
            }
            uiImTooltip(_("Refresh"));

            uiImSameLine(0, 4);

            if (uiImButton(__("\ue161"))) { //Save
                try {
                item.saveBindings();
                } catch (Exception ex) {
                    uiImDialog(__("Error"), ex.msg);
                }
            }
            uiImTooltip(_("Save to File"));

            foreach(i, ref TrackingBinding binding; item.bindings) {
                uiImPush(&binding);
                    if (uiImBeginCategory(binding.name.toStringz)) {
                        switch(binding.type) {

                            case BindingType.RatioBinding:
                                ratioBinding(i, binding.delegated);
                                break;

                            case BindingType.ExpressionBinding:
                                exprBinding(i, binding.delegated);
                                break;

                            case BindingType.EventBinding:
                                eventBinding(i, binding.delegated);
                                break;

                            case BindingType.CompoundBinding:
                                compoundBinding(i, binding.delegated);
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
    this() {
        super("Tracking", _("Tracking"), true);
    }
}

mixin inPanel!TrackingPanel;
