/*
    Copyright © 2022, nijigenerate Project
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

    struct TrackingSource {
        bool isBone;
        string name;
        const(char)* cName;
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
    if (insSceneSelectedSceneItem()) {
        foreach(ref TrackingBinding binding; insSceneSelectedSceneItem().bindings) {
            paramNames ~= binding.param.name.toStringz;
        }
    }
}

class TrackingPanel : Panel {
private:
    TrackingSource[] sources;

    // Refreshes the list of tracking sources
    void refresh(ref TrackingBinding[] trackingBindings) {
        auto blendshapes = insScene.space.getAllBlendshapeNames();
        auto bones = insScene.space.getAllBoneNames();
        
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

            indexableSourceNames[blendshapes.length+i] = bone.toLower;
            minValues[i] = -1;
            maxValues[i] = 1;
        }

        // Add any bindings unnacounted for which are stored in the model.
        trkMain: foreach(bind; trackingBindings) {
            
            // Skip non-existent sources
            if (bind.sourceName.length == 0) continue;

            TrackingSource src = TrackingSource(
                bind.sourceType != SourceType.Blendshape,
                bind.sourceName,
                bind.sourceName.toStringz
            );

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

    
    // Settings popup for binding types
    pragma(inline, true)
    bool settingsPopup(ref TrackingBinding binding) {
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
    void exprBinding(size_t i, ref TrackingBinding binding) {
        auto eBinding = cast(ExpressionTrackingBinding)binding.delegated;
        if (eBinding.expr) {
            string buf = eBinding.expr.expression.dup;
            if (settingsPopup(binding))
                return;
            
            uiImLabel(_("Dampen"));
            igSliderInt("", &binding.dampenLevel, 0, 10);

            if (uiImInputText("###EXPRESSION", buf)) {
                eBinding.expr.expression = buf.toStringz.fromStringz;
            }

            uiImLabel(_("Output (%s)").format(binding.outVal));
            uiImIndent();
                uiImProgress(binding.outVal);
            

                uiImPushTextWrapPos();
                    if (eBinding.expr.lastError.length > 0) {
                        uiImLabelColored(eBinding.expr.lastError, vec4(1, 0.4, 0.4, 1));
                        uiImNewLine();
                    }

                    if (binding.outVal < 0 || binding.outVal > 1) {
                        uiImLabelColored(_("Value out of range, clamped to 0..1 range."), vec4(0.95, 0.88, 0.62, 1));
                        uiImNewLine();
                    }
                uiImPopTextWrapPos();
            uiImUnindent();
        }
    }

    // Configuration panel for ratio bindings
    void ratioBinding(size_t i, ref TrackingBinding binding) {
        bool hasTrackingSrc = binding.sourceName.length > 0;

        if (settingsPopup(binding))
            return;
        igSameLine();
        if (uiImBeginComboBox("SELECTION_COMBO", hasTrackingSrc ? binding.sourceDisplayName.toStringz : __("Not tracked"))) {
            string filter = trackingFilter.dup;
            if (uiImInputText("###FILTER", uiImAvailableSpace().x, filter)) {
                trackingFilter = filter.toLower().toStringz.fromStringz;
            }

            uiImDummy(vec2(0, 8));

            
            foreach(ix, source; sources) {
                
                if (trackingFilter.length > 0 && !indexableSourceNames[ix].canFind(trackingFilter)) continue;

                bool selected = binding.sourceName == source.name;
                bool nameValid = source.name.length > 0;
                if (source.isBone) {
                    if (uiImBeginMenu(source.cName)) {
                        if (uiImMenuItem(__("X"))) {
                            binding.sourceName = source.name;
                            binding.sourceType = SourceType.BonePosX;
                            binding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Y"))) {
                            binding.sourceName = source.name;
                            binding.sourceType = SourceType.BonePosY;
                            binding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Z"))) {
                            binding.sourceName = source.name;
                            binding.sourceType = SourceType.BonePosZ;
                            binding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Roll"))) {
                            binding.sourceName = source.name;
                            binding.sourceType = SourceType.BoneRotRoll;
                            binding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Pitch"))) {
                            binding.sourceName = source.name;
                            binding.sourceType = SourceType.BoneRotPitch;
                            binding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        if (uiImMenuItem(__("Yaw"))) {
                            binding.sourceName = source.name;
                            binding.sourceType = SourceType.BoneRotYaw;
                            binding.createSourceDisplayName();
                            trackingFilter = null;
                        }
                        uiImEndMenu();
                    }
                } else {
                    if (igSelectable(nameValid ?source.cName : "###NoName", selected, ImGuiSelectableFlags.SpanAllColumns | ImGuiSelectableFlags.AllowItemOverlap)) {
                        trackingFilter = null;
                        binding.sourceType = SourceType.Blendshape;
                        binding.sourceName = source.name;
                        binding.createSourceDisplayName();
                    }
                    igSameLine();
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
            binding.sourceName = null;
        }

        if (hasTrackingSrc) {
            auto rBinding = cast(RatioTrackingBinding)binding.delegated;
            uiImCheckbox(__("Inverse"), rBinding.inverse);

            uiImLabel(_("Dampen"));
            igSliderInt("", &binding.dampenLevel, 0, 10);

            uiImLabel(_("Tracking In"));
            uiImPush(0);
                uiImIndent();
                    igSetNextItemWidth (96);
                    switch(binding.sourceType) {
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
                    uiImProgress(binding.inVal, vec2(-float.min_normal, 0), "");
                uiImUnindent();
            uiImPop();
            
            uiImLabel(_("Tracking Out"));
            uiImPush(1);
                uiImIndent();
                    igSetNextItemWidth (96);
                    uiImRange(rBinding.outRange.x, rBinding.outRange.y, -float.max, float.max);
                    igSameLine();
                    uiImProgress(binding.param.mapAxis(binding.axis, binding.outVal), vec2(-float.min_normal, 0), "");
                uiImUnindent();
            uiImPop();
        }
    }

    // Configuration panel for event bindings
    void eventBinding(size_t i, ref TrackingBinding binding) {
        auto eBinding = cast(EventTrackingBinding)binding.delegated;
        if (eBinding) {
            if (settingsPopup(binding))
                return;

            uiImLabel(_("Dampen"));
            igSliderInt("", &binding.dampenLevel, 0, 10);

            int indexToRemove = -1;
            foreach (idx, item; eBinding.valueMap) {
                uiImPush(cast(int)idx + 1);
                string idHold = item.id.dup;
                if (uiImInputText("###EVENTID", 64, idHold)) {
                    eBinding.valueMap[idx].id = idHold.toStringz.fromStringz.toUpper();
                }
                igSameLine();
                igSetNextItemWidth(128);
                igDragFloat("", &(eBinding.valueMap[idx].value), 0.01, binding.param.min.vector[binding.axis], binding.param.max.vector[binding.axis]);
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
                if (uiImButton(__("+"))) {
                    eBinding.valueMap ~= EventTrackingBinding.EventMap(SourceType.KeyPress, "", 0);
                }
            }

            uiImLabel(_("Output (%s)").format(binding.outVal));
            uiImIndent();
                uiImProgress(binding.outVal);
            
                uiImPushTextWrapPos();
                    if (binding.outVal < 0 || binding.outVal > 1) {
                        uiImLabelColored(_("Value out of range, clamped to 0..1 range."), vec4(0.95, 0.88, 0.62, 1));
                        uiImNewLine();
                    }
                uiImPopTextWrapPos();
            uiImUnindent();
        }
    }

protected:

    override 
    void onUpdate() {
        auto item = insSceneSelectedSceneItem();
        if (item) {
            if (indexableSourceNames.length == 0 || uiImButton(__("Refresh"))) {
                insScene.space.refresh();
                refresh(item.bindings);
            }

            uiImSameLine(0, 4);

            if (uiImButton(__("Save to File"))) {
                try {
                item.saveBindings();
                } catch (Exception ex) {
                    uiImDialog(__("Error"), ex.msg);
                }
            }

            foreach(i, ref TrackingBinding binding; item.bindings) {
                uiImPush(&binding);
                    if (uiImHeader(binding.name.toStringz, true)) {
                        uiImIndent();
                            switch(binding.type) {

                                case BindingType.RatioBinding:
                                    ratioBinding(i, binding);
                                    break;

                                case BindingType.ExpressionBinding:
                                    exprBinding(i, binding);
                                    break;

                                case BindingType.EventBinding:
                                    eventBinding(i, binding);
                                    break;

                                // External bindings
                                default: 
                                    settingsPopup(binding);
                                    uiImLabel(_("No settings available."));
                                    break;
                            }
                        uiImUnindent();
                    }
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