/*
    Copyright © 2022, Inochi2D Project
    Copyright © 2024, nijigenerate Project
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

    void adaptorDelete(size_t idx) {
        // stop source on delete
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
                
                // stop ALL sources on delete
                foreach(ref source; insScene.space.getZones()[idx].sources) {
                    if (source) source.stop();
                }
                insScene.space.removeZoneAt(idx);
            }
            uiImEndPopup();
        }
    }

    /**
        this function show hints to help users to configure the adaptor correctly
    */
    void adaptorHint(ref Adaptor source) {
        /**
            for port binding information, we should refer to `facetrack-d/source/ft/adaptors/*.d`
        */

        string portHint = "";
        if (auto vts = cast(IFMAdaptor) source)
            portHint = _("iFacialMocap Adpator would listen on udp port 49983");
        if (portHint.length > 0)
            uiImLabel(portHint ~ "\n" ~ _("Make sure the port is not blocked by firewall."));
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
    void onBeginUpdate() {
        ImVec2 wpos = ImVec2(
            igGetMainViewport().Pos.x+(igGetMainViewport().Size.x/2),
            igGetMainViewport().Pos.y+(igGetMainViewport().Size.y/2),
        );

        ImVec2 uiSize = ImVec2(
            800, 
            600
        );

        igSetNextWindowPos(wpos, ImGuiCond.Appearing, ImVec2(0.5, 0.5));
        igSetNextWindowSize(uiSize, ImGuiCond.Appearing);
        igSetNextWindowSizeConstraints(uiSize, ImVec2(float.max, float.max));
        super.onBeginUpdate();
    }

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

            string editingZoneName = newZoneName.dup;
            if (uiImInputText("###ZONE_NAME", avail.x-24, editingZoneName)) {
                try {
                    newZoneName = editingZoneName.toStringz.fromStringz;
                } catch (std.utf.UTFException e) {}
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
                    string editingZoneName = editingZone.name;
                    if (uiImInputText("###ZoneName", avail.x/2, editingZoneName)) {
                        try {
                            editingZone.name = editingZoneName.toStringz.fromStringz;
                        } catch (std.utf.UTFException e) {}
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

                                            adaptorHint(source);

                                            if (uiImButton(__("Save Changes"))) {
                                                try {
                                                    source.setOptions(options[source]);
                                                    source.stop();
                                                    source.start();
                                                } catch(Exception ex) {
                                                    uiImDialog(__("Error"), ex.msg);
                                                }
                                            }

                                            // Expose the delete button to make sure users notice it.
                                            uiImSameLine(0, 40);
                                            if (uiImButton(__("Delete")))
                                                adaptorDelete(i);
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

        uiImDummy(vec2(-132, 0));
        uiImSameLine(0, 0);
        if (uiImButton(__("Cancel"), vec2(64, 0))) {
            this.close();
        }
        uiImSameLine(0, 4);
        if (uiImButton(__("Save"), vec2(64, 0))) {
            insSaveVSpace(insScene.space);
            neTrackingPanelReset();
            this.close();
        }
    }

    this() {
        super(_("Virtual Space"));
        flags = ImGuiWindowFlags.NoTitleBar | ImGuiWindowFlags.NoResize |
                ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoSavedSettings |
                ImGuiWindowFlags.NoScrollbar;
    }
}
