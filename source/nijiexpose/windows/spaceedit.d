/*
    Copyright © 2022, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.windows.spaceedit;
import nijiexpose.scene;
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

import std.algorithm.mutation;

class SpaceEditor : ToolWindow {
private:
    VirtualSpaceZone editingZone;
    string newZoneName;

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

    void refreshOptionsList() {
        options.clear();

        foreach(i; 0..editingZone.sources.length) {

            if (editingZone.sources[i] is null) continue;
            
            options[editingZone.sources[i]] = editingZone.sources[i].getOptions();
            options[editingZone.sources[i]]["appName"] = "nijiexpose";
        }
    }

    string[string][Adaptor] options;

    void adaptorMenu(size_t idx) {
        uiImRightClickPopup("AdaptorPopup");

        if (uiImBeginPopup("AdaptorPopup")) {
            if (uiImMenuItem(__("Delete"))) {

                // stop source on delete
                if (editingZone.sources[idx]) editingZone.sources[idx].stop();
                editingZone.sources = editingZone.sources.remove(idx);
            }
            uiImEndPopup();
        }
    }

    void zoneMenu(size_t idx) {
        uiImRightClickPopup("AdaptorPopup");


        if (uiImBeginPopup("AdaptorPopup")) {
            if (uiImMenuItem(__("Delete"))) {
                
                // stop ALL sources on delete
                foreach(ref source; insScene.space.getZones()[idx].sources) {
                    if (source) source.stop();
                }
                insScene.space.removeZoneAt(idx);
            }
            uiImEndPopup();
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

    override
    void onUpdate() {
        vec2 avail = uiImAvailableSpace();
        float lhs = 196;
        float rhs = avail.x-lhs;

        if (uiImBeginChild("##LHS", vec2(lhs, -28), true)) {
            avail = uiImAvailableSpace();
            foreach(i, ref VirtualSpaceZone zone; insScene.space.getZones()) {
                uiImPush(cast(int)i);
                    if (uiImSelectable(zone.name.toStringz, zone == editingZone)) {
                        switchZone(zone);
                    }
                    zoneMenu(i);
                uiImPop();
            }

            if (uiImInputText("###ZONE_NAME", avail.x-24, newZoneName)) {
                newZoneName = newZoneName.toStringz.fromStringz;
            }
            uiImSameLine(0, 0);
            if (uiImButton(__(""), vec2(24, 24))) addZone();
        }
        uiImEndChild();

        uiImSameLine(0, 0);

        if (uiImBeginChild("##RHS", vec2(rhs, -28), true)) {
            if (editingZone is null) {
                uiImLabel(_("No zone selected for editing..."));
            } else {
                uiImPush(cast(int)editingZone.hashOf());
                    if (uiImInputText("###ZoneName", avail.x/2, editingZone.name)) {
                        editingZone.name = editingZone.name.toStringz.fromStringz;
                    }

                    uiImSeperator();
                    uiImNewLine();

                    uiImIndent();
                        foreach(i; 0..editingZone.sources.length) {
                            // Deleting a source causes some confusion here.
                            if (i >= editingZone.sources.length) {
                                continue;
                            }

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

                                            foreach(option; source.getOptionNames()) {
                                                
                                                // Skip options which shouldn't be shown
                                                if (option == "appName") continue;
                                                if (option == "address") continue;

                                                if (option !in options[source]) options[source][option] = "";
                                                uiImLabel(option);
                                                string optionString = options[source][option].dup;
                                                if (uiImInputText(option, avail.x/2, optionString)) {
                                                    options[source][option] = optionString.toStringz.fromStringz;
                                                }
                                            }

                                            if (uiImButton(__("Save Changes"))) {
                                                try {
                                                    source.setOptions(options[source]);
                                                    source.stop();
                                                    source.start();
                                                } catch(Exception ex) {
                                                    uiImDialog(__("Error"), ex.msg);
                                                }
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
                    if (uiImButton(__(""), vec2(avail.x, 24))) {
                        editingZone.sources.length++;
                        refreshOptionsList();
                    }
                uiImPop();
            }
        }
        uiImEndChild();

        uiImDummy(vec2(-64, 0));
        uiImSameLine(0, 0);
        if (uiImButton(__("Save"), vec2(64, 0))) {
            insSaveVSpace(insScene.space);
            neTrackingPanelReset();
            this.close();
        }
    }

    this() {
        super(_("Virtual Space"));
        flags |= ImGuiWindowFlags.NoScrollbar;
    }
}
