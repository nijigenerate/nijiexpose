/*
    Copyright ﾂｩ 2022, Inochi2D Project
    Copyright ﾂｩ 2024, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.

    Authors: Luna Nielsen
*/
module nijiexpose.windows.spaceedit;
import nijiexpose.scene;
import nijiexpose.tracking.tracker : neTracker;
import nijiexpose.tracking.vspace;
import nijiexpose.panels.tracking;
import nijiexpose.log;
import nijiui.widgets;
import nijiui.toolwindow;
import inmath;
import i18n;
import std.string;
import bindbc.imgui;
import ft;
static import std.utf;

import std.algorithm.mutation;
import std.algorithm.comparison : max;

class SpaceEditor : ToolWindow {
private:
    VirtualSpaceZone editingZone;
    string newZoneName;
    string[string][Adaptor] options;

    void addZone() {
        if (newZoneName.length == 0) {
            uiImDialog(__("Error"), "Can't create a zone without a name!");
            return;
        }
        newZoneName = newZoneName.toStringz.fromStringz;

        VirtualSpaceZone zone = new VirtualSpaceZone(newZoneName.dup);
        insScene.space.addZone(zone);

        newZoneName = null;
    }

    void switchZone(VirtualSpaceZone zone) {
        editingZone = zone;
        refreshOptionsList();
    }

    void ensureEditingZone() {
        if (editingZone !is null) return;
        auto zones = insScene.space.getZones();
        if (zones.length > 0) {
            switchZone(zones[0]);
        }
    }

    void refreshOptionsList() {
        options.clear();
        if (editingZone is null) return;

        foreach (i; 0 .. editingZone.sources.length) {
            if (editingZone.sources[i] is null) continue;
            options[editingZone.sources[i]] = editingZone.sources[i].getOptions();
            options[editingZone.sources[i]]["appName"] = "nijiexpose";
        }
    }

    void adaptorDelete(size_t idx) {
        if (editingZone.sources[idx]) editingZone.sources[idx].stop();
        editingZone.sources = editingZone.sources.remove(idx);
    }

    void adaptorMenu(size_t idx) {
        uiImRightClickPopup("AdaptorPopup");

        if (uiImBeginPopup("AdaptorPopup")) {
            if (uiImMenuItem(__("Delete")))
                adaptorDelete(idx);
            uiImEndPopup();
        }
    }

    void zoneMenu(size_t idx) {
        uiImRightClickPopup("AdaptorPopup");

        if (uiImBeginPopup("AdaptorPopup")) {
            if (uiImMenuItem(__("Delete"))) {
                foreach (ref source; insScene.space.getZones()[idx].sources) {
                    if (source) source.stop();
                }
                insScene.space.removeZoneAt(idx);
            }
            uiImEndPopup();
        }
    }

    void adaptorHint(ref Adaptor source) {
        string portHint = "";
        if (auto vts = cast(IFMAdaptor)source) {
            portHint = _("iFacialMocap Adpator would listen on udp port 49983");
        }
        if (portHint.length > 0) {
            uiImLabel(portHint ~ "\n" ~ _("Make sure the port is not blocked by firewall."));
        }
    }

    void adaptorSelect(size_t i, ref Adaptor source, const(char)* adaptorName) {
        if (uiImBeginComboBox("ADAPTOR_COMBO", adaptorName)) {
            if (uiImSelectable("VTubeStudio")) {
                if (source) source.stop();
                source = new VTSAdaptor();
                editingZone.sources[i] = source;
                refreshOptionsList();
            }
            if (uiImSelectable("VMC")) {
                if (source) source.stop();
                source = new VMCAdaptor();
                editingZone.sources[i] = source;
                refreshOptionsList();
            }
            if (uiImSelectable("Phiz OSC")) {
                if (source) source.stop();
                source = new PhizOSCAdaptor();
                editingZone.sources[i] = source;
                refreshOptionsList();
            }
            if (uiImSelectable("OpenSeeFace")) {
                if (source) source.stop();
                source = new OSFAdaptor();
                editingZone.sources[i] = source;
                refreshOptionsList();
            }
            if (uiImSelectable("iFacialMocap")) {
                if (source) source.stop();
                source = new IFMAdaptor();
                editingZone.sources[i] = source;
                refreshOptionsList();
            }
            if (uiImSelectable("Facemotion3D")) {
                if (source) source.stop();
                source = new FM3DAdaptor();
                editingZone.sources[i] = source;
                refreshOptionsList();
            }
            version (WebHookAdaptor) {
                if (uiImSelectable("WebHook Receiver")) {
                    if (source) source.stop();
                    source = new WebHookAdaptor();
                    editingZone.sources[i] = source;
                    refreshOptionsList();
                }
            }
            version (Phiz) {
                if (uiImSelectable("Phiz Receiver")) {
                    if (source) source.stop();
                    source = new PhizAdaptor();
                    editingZone.sources[i] = source;
                    refreshOptionsList();
                }
            }
            version (JML) {
                if (uiImSelectable("JINS MEME Logger")) {
                    if (source) source.stop();
                    source = new JMLAdaptor();
                    editingZone.sources[i] = source;
                    refreshOptionsList();
                }
            }
            uiImEndComboBox();
        }
    }

public:
    void applyChanges() {
        insScene.space.refresh();
        insSaveVSpace(insScene.space);
        neTrackingPanelReset();
        insTrackingPanelRefresh();
        if (neTracker.enabled) {
            neTracker.setupVSpace();
        }
    }

    void renderEditorSection(bool showApplyButton = false) {
        vec2 avail = uiImAvailableSpace();
        float lhs = 196;
        float rhs = avail.x - lhs;
        immutable float footerHeight = showApplyButton ? 38.0f : 0.0f;

        igPushStyleColor(ImGuiCol.ChildBg, ImVec4(1, 1, 1, 0));
        igPushStyleVar(ImGuiStyleVar.ChildBorderSize, 0.0f);
        scope(exit) {
            igPopStyleVar();
            igPopStyleColor();
        }

        if (uiImBeginChild("##LHS", vec2(lhs, -footerHeight), false)) {
            avail = uiImAvailableSpace();
            foreach (i, ref VirtualSpaceZone zone; insScene.space.getZones()) {
                uiImPush(cast(int)i);
                    if (uiImSelectable(zone.name.toStringz, zone == editingZone)) {
                        switchZone(zone);
                    }
                    zoneMenu(i);
                uiImPop();
            }

            string editingZoneName = newZoneName.dup;
            if (uiImInputText("###ZONE_NAME", avail.x - 24, editingZoneName)) {
                try {
                    newZoneName = editingZoneName.toStringz.fromStringz;
                } catch (std.utf.UTFException e) {}
            }
            uiImSameLine(0, 0);
            if (uiImButton(__("\ue145"), vec2(24, 24))) addZone();
        }
        uiImEndChild();

        uiImSameLine(0, 0);

        if (uiImBeginChild("##RHS", vec2(rhs, -footerHeight), false)) {
            if (editingZone is null) {
                uiImLabel(_("No zone selected for editing..."));
            } else {
                uiImPush(cast(int)editingZone.hashOf());
                    string editingZoneName = editingZone.name;
                    if (uiImInputText("###ZoneName", avail.x / 2, editingZoneName)) {
                        try {
                            editingZone.name = editingZoneName.toStringz.fromStringz;
                        } catch (std.utf.UTFException e) {}
                    }

                    uiImSeperator();
                    uiImNewLine();

                    uiImIndent();
                        foreach (i; 0 .. editingZone.sources.length) {
                            if (i >= editingZone.sources.length) continue;

                            uiImPush(cast(int)i);
                                auto source = editingZone.sources[i];
                                const(char)* adaptorName = source is null ? __("Unset") : source.getAdaptorName().toStringz;

                                if (source is null) {
                                    if (uiImHeader(adaptorName, true)) {
                                        adaptorMenu(i);
                                        uiImIndent();
                                            adaptorSelect(i, source, adaptorName);
                                        uiImUnindent();
                                    } else {
                                        adaptorMenu(i);
                                    }
                                } else {
                                    if (uiImHeader(adaptorName, true)) {
                                        adaptorMenu(i);
                                        uiImIndent();
                                            avail = uiImAvailableSpace();
                                            adaptorSelect(i, source, adaptorName);
                                            igNewLine();

                                            foreach (option; source.getOptionNames()) {
                                                if (option == "appName") continue;
                                                if (option == "address") continue;

                                                if (option !in options[source]) options[source][option] = "";
                                                uiImLabel(option);
                                                string optionString = options[source][option].dup;
                                                if (uiImInputText(option, avail.x / 2, optionString)) {
                                                    options[source][option] = optionString.toStringz.fromStringz;
                                                }
                                            }

                                            adaptorHint(source);

                                            if (uiImButton(__("Save Changes"))) {
                                                try {
                                                    source.setOptions(options[source]);
                                                    source.stop();
                                                    source.start();
                                                } catch (Exception ex) {
                                                    uiImDialog(__("Error"), ex.msg);
                                                }
                                            }

                                            uiImSameLine(0, 40);
                                            if (uiImButton(__("Delete"))) {
                                                adaptorDelete(i);
                                            }
                                        uiImUnindent();
                                    } else {
                                        adaptorMenu(i);
                                    }
                                }
                            uiImPop();
                        }
                    uiImUnindent();

                    avail = uiImAvailableSpace();
                    if (uiImButton(__("\ue145"), vec2(avail.x, 24))) {
                        editingZone.sources.length++;
                        refreshOptionsList();
                    }
                uiImPop();
            }
        }
        uiImEndChild();

        if (showApplyButton) {
            igSetCursorPosX(max(0.0f, uiImAvailableSpace().x - 72.0f));
            if (uiImButton(__("Apply"), vec2(64, 0))) {
                applyChanges();
            }
        }
    }

    void renderEmbeddedSection() {
        ensureEditingZone();

        auto zones = insScene.space.getZones();
        if (uiImBeginCategory(_("Zone").toStringz, UiImCategoryFlags.NoCollapse)) {
            int zoneToDelete = -1;
            foreach (idx, zone; zones) {
                uiImPush(cast(int)idx);
                    vec2 avail = uiImAvailableSpace();
                    float rowWidth = max(0.0f, avail.x - 38.0f);
                    bool selected = zone == editingZone;
                    if (igSelectable(zone.name.toStringz, selected, ImGuiSelectableFlags.None, ImVec2(rowWidth, 0))) {
                        switchZone(zone);
                    }
                    uiImSameLine();
                    if (uiImButton(__("\ue872"), vec2(28, 0))) {
                        zoneToDelete = cast(int)idx;
                    }
                uiImPop();
            }

            if (zoneToDelete >= 0) {
                auto zonesNow = insScene.space.getZones();
                bool deletedSelected = editingZone is zonesNow[zoneToDelete];
                foreach (ref source; zonesNow[zoneToDelete].sources) {
                    if (source) source.stop();
                }
                insScene.space.removeZoneAt(cast(size_t)zoneToDelete);
                if (deletedSelected) {
                    editingZone = null;
                }
                ensureEditingZone();
                refreshOptionsList();
                uiImEndCategory();
                return;
            }

            string pendingZoneName = newZoneName.dup;
            vec2 avail = uiImAvailableSpace();
            if (uiImInputText("##EMBEDDED_ZONE_NAME", max(0.0f, avail.x - 38.0f), pendingZoneName)) {
                try {
                    newZoneName = pendingZoneName.toStringz.fromStringz;
                } catch (std.utf.UTFException e) {}
            }
            uiImSameLine();
            if (uiImButton(__("\ue145"), vec2(28, 0))) {
                addZone();
                ensureEditingZone();
            }

            if (editingZone is null) {
                uiImLabel(_("No zone selected for editing..."));
                uiImEndCategory();
                return;
            }
            uiImEndCategory();
        }

        foreach (i; 0 .. editingZone.sources.length) {
            if (i >= editingZone.sources.length) continue;

            uiImPush(cast(int)i);
                auto source = editingZone.sources[i];
                const(char)* adaptorName = source is null ? __("Unset") : source.getAdaptorName().toStringz;

                if (source is null) {
                    if (uiImBeginCategory(adaptorName)) {
                        adaptorMenu(i);
                        ImVec2 headerStart;
                        igGetItemRectMin(&headerStart);
                        ImVec2 headerEnd;
                        igGetItemRectMax(&headerEnd);
                        float deleteX = headerEnd.x - 32.0f;
                        float iconY = headerStart.y + 1.0f;
                        igSetCursorScreenPos(ImVec2(deleteX, iconY));
                        if (uiImButton(__("\ue872"), vec2(28, 0))) {
                            adaptorDelete(i);
                            uiImEndCategory();
                            uiImPop();
                            continue;
                        }
                        adaptorSelect(i, source, adaptorName);
                    }
                    uiImEndCategory();
                } else {
                    if (uiImBeginCategory(adaptorName)) {
                        adaptorMenu(i);
                        ImVec2 headerStart;
                        igGetItemRectMin(&headerStart);
                        ImVec2 headerEnd;
                        igGetItemRectMax(&headerEnd);
                        float iconY = headerStart.y + 1.0f;
                        float deleteX = headerEnd.x - 32.0f;
                        float saveX = deleteX - 32.0f;

                        igSetCursorScreenPos(ImVec2(saveX, iconY));
                        if (uiImButton(__("\ue161"), vec2(28, 0))) {
                            try {
                                source.setOptions(options[source]);
                                source.stop();
                                source.start();
                            } catch (Exception ex) {
                                uiImDialog(__("Error"), ex.msg);
                            }
                        }
                        igSetCursorScreenPos(ImVec2(deleteX, iconY));
                        if (uiImButton(__("\ue872"), vec2(28, 0))) {
                            adaptorDelete(i);
                            uiImEndCategory();
                            uiImPop();
                            continue;
                        }

                        vec2 avail = uiImAvailableSpace();
                        adaptorSelect(i, source, adaptorName);
                        igNewLine();

                        foreach (option; source.getOptionNames()) {
                            if (option == "appName") continue;
                            if (option == "address") continue;

                            if (option !in options[source]) options[source][option] = "";
                            uiImLabel(option);
                            string optionString = options[source][option].dup;
                            if (uiImInputText(option, avail.x / 2, optionString)) {
                                options[source][option] = optionString.toStringz.fromStringz;
                            }
                        }

                        adaptorHint(source);
                    }
                    uiImEndCategory();
                }
            uiImPop();
        }

        vec2 avail = uiImAvailableSpace();
        if (uiImButton(__("Add Source"), vec2(min(avail.x, 220.0f), 28))) {
            editingZone.sources.length++;
            refreshOptionsList();
        }
    }

    override
    void onBeginUpdate() {
        ImVec2 wpos = ImVec2(
            igGetMainViewport().Pos.x + (igGetMainViewport().Size.x / 2),
            igGetMainViewport().Pos.y + (igGetMainViewport().Size.y / 2),
        );

        ImVec2 uiSize = ImVec2(800, 600);

        igSetNextWindowPos(wpos, ImGuiCond.Appearing, ImVec2(0.5, 0.5));
        igSetNextWindowSize(uiSize, ImGuiCond.Appearing);
        igSetNextWindowSizeConstraints(uiSize, ImVec2(float.max, float.max));
        super.onBeginUpdate();
    }

    override
    void onUpdate() {
        renderEditorSection(true);
    }

    this() {
        super(_("Virtual Space"));
        flags = ImGuiWindowFlags.NoTitleBar | ImGuiWindowFlags.NoResize |
                ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoSavedSettings |
                ImGuiWindowFlags.NoScrollbar;
    }
}
