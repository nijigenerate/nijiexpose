module nijiexpose.windows.settings;

import nijiexpose.log;
import nijiexpose.scene;
import nijiexpose.tracking.tracker;
import nijiexpose.windows.main;
import nijiui.core.settings;
import nijiui.widgets;
import nijiui.toolwindow;
import inmath;
import i18n;
import std.string;
import bindbc.imgui;
import ft;
import std.algorithm;
import std.file;
import nijiexpose.utils.subprocess;
import nijilive.core.nodes.common : nlIsTripleBufferFallbackEnabled, nlSetTripleBufferFallback;

import std.algorithm.mutation;

class SettingWindow : ToolWindow {
private:
    bool prevMeasureFPS;
    Tracker trackerDraft;
    bool trackerDraftLoaded = false;
    int throttlingDraft = 1;
    bool tripleFallbackDraft = false;

    void copyTrackerState(ref Tracker dst, ref Tracker src) {
        dst.enabled = src.enabled;
        dst.flipped = src.flipped;
        dst.showWindow = src.showWindow;
        dst.hostname = src.hostname;
        dst.port = src.port;
        dst.device = src.device;
        dst.trackerPath = src.trackerPath;
    }

    void ensureDraftLoaded() {
        if (trackerDraftLoaded) return;
        auto tracker = neTracker();
        if (trackerDraft is null) {
            trackerDraft = new Tracker();
        }
        copyTrackerState(trackerDraft, tracker);
        throttlingDraft = inSettingsGet!int("throttlingRate", 1);
        tripleFallbackDraft = inSettingsGet!bool("TripleBufferFallback", nlIsTripleBufferFallbackEnabled());
        trackerDraftLoaded = true;
    }

public:
    void applyTrackingSettings() {
        ensureDraftLoaded();
        auto tracker = neTracker();
        copyTrackerState(tracker, trackerDraft);
        inSettingsSet("tracker", tracker);
        inSettingsSave();
        if (tracker.enabled) {
            tracker.restart();
            tracker.setupVSpace();
        } else {
            tracker.terminate();
        }
    }

    void applyRenderingSettings() {
        ensureDraftLoaded();
        inSettingsSet("throttlingRate", throttlingDraft);
        neWindowSetThrottlingRate(throttlingDraft);
        inSettingsSet("TripleBufferFallback", tripleFallbackDraft);
        nlSetTripleBufferFallback(tripleFallbackDraft);
        inSettingsSave();
    }

    void applySettings() {
        applyTrackingSettings();
        applyRenderingSettings();
    }
    SelectedMode selected = SelectedMode.Tracking;
    string pythonPath = null;
    bool pythonPathTested = false;

    override
    void onBeginUpdate() {
        neSetMeasureFPS(true);
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

    enum SelectedMode {
        Tracking,
        Rendering
    }

    void renderTrackingSettingsContent(bool embedded) {
        ensureDraftLoaded();
        auto tracker = trackerDraft;
        uiImCheckbox(__("Enable tracking"), tracker.enabled);
        if (tracker.enabled) {
            tracker.update();
            bool openedEmbeddedCategory = false;
            if (embedded) {
                openedEmbeddedCategory = uiImBeginCategory("##tracker");
            }
            if (!embedded || openedEmbeddedCategory) {
                uiImLabel(_("Python path"));
                uiImSameLine();
                if (!pythonPathTested) {
                    pythonPath = PythonProcess!false.detectPython();
                    pythonPathTested = true;
                }
                if (pythonPath is null) {
                    uiImLabelColored(_("Python is not detected. Please install python (<=3.12) first."), vec4(0.9, 0.5, 0.5, 1));
                } else {
                    uiImLabel(pythonPath);
                }
                uiImLabel(_("Tracker executable path"));
                uiImSameLine();
                uiImInputText("##trackerPath", tracker.trackerPath);
                if (!tracker.scriptPath.exists) {
                    uiImLabelColored(_("Specified path doesn't contain %s").format(tracker.trackerScriptName), vec4(0.95, 0.5, 0.5, 1));
                    uiImSameLine();
                    if (uiImButton(__("Install"))) {
                        tracker.install();
                    }
                } else if (tracker.installProcess !is null) {
                    tracker.installProcess.update();
                    auto outputText = tracker.installProcess.stdoutOutput.join("\n");
                    vec2 avail2 = uiImAvailableSpace();
                    vec2 logAreaSize = vec2(avail2.x, avail2.y - 50);
                    if (uiImBeginChild("##log_area", logAreaSize, true)) {
                        igTextUnformatted(outputText.toStringz);
                    }
                    uiImEndChild();
                    if (!tracker.installProcess.running()) {
                        if (uiImButton(__("OK"))) {
                            tracker.installProcess = null;
                        }
                    }
                } else {
                    uiImLabel(_("Camera device"));
                    uiImSameLine();
                    auto deviceList = tracker.listDevices();
                    long currentDeviceId = deviceList.countUntil!(x=>x.id == tracker.device)();
                    string currentDeviceName = (currentDeviceId >= 0)? deviceList[currentDeviceId].name: _("Select device...");
                    if (igBeginCombo("##device", currentDeviceName.toStringz ,ImGuiComboFlags.None)) {
                        if (deviceList !is null) {
                            foreach (device; deviceList) {
                                if (uiImSelectable(device.name.toStringz, device.id == tracker.device)) {
                                    tracker.device = device.id;
                                }
                            }
                        }
                        igEndCombo();
                    }
                    uiImLabel(_("Host name"));
                    uiImSameLine();
                    uiImInputText("##host", tracker.hostname);
                    uiImLabel(_("Port number"));
                    uiImSameLine();
                    igInputInt("##PortNumber", cast(int*)&tracker.port);
                    uiImCheckbox(__("Flip input"), tracker.flipped);
                    uiImCheckbox(__("Show camera tracking window"), tracker.showWindow);
                }
                if (embedded) {
                    igDummy(ImVec2(0, 4));
                }
                if (openedEmbeddedCategory) {
                    uiImEndCategory();
                }
            }
        }
    }

    void renderTrackingSettingsSection() {
        if (uiImHeader(__("Tracking"), true)) {
            uiImIndent();
                renderTrackingSettingsContent(false);
            uiImUnindent();
        }
    }

    void renderRenderingSettingsContent(bool embedded) {
        ensureDraftLoaded();
        if (embedded) {
            uiImLabelColored(_("V-Sync throttling"), vec4(0.8, 0.3, 0.3, 1));
            uiImSeperator();
            uiImIndent();
                uiImLabel("%s (%s)".format(_("Throtting interval"), _("Experimental")));
                igSliderInt("##THROTTLING", &throttlingDraft, 0, 6);
                uiImSameLine();
                uiImLabel(_("Frame rate: %.2f fps".format(neGetFPS())));
            uiImUnindent();
            igDummy(ImVec2(0, 6));

            uiImLabelColored(_("Triple buffer fallback"), vec4(0.8, 0.3, 0.3, 1));
            uiImSeperator();
            uiImIndent();
                uiImCheckbox(__("Enable triple buffer fallback"), tripleFallbackDraft);
                uiImLabel(_("Use when advanced blend is unavailable or causes issues."));
            uiImUnindent();
            return;
        }

        if (uiImHeader(__("V-Sync throttling"), true)) {
            uiImIndent();
                uiImLabel("%s (%s)".format(_("Throtting interval"), _("Experimental")));
                igSliderInt("##THROTTLING", &throttlingDraft, 0, 6);
                uiImSameLine();
                uiImLabel(_("Frame rate: %.2f fps".format(neGetFPS())));
            uiImUnindent();
        }
        if (uiImHeader(__("Triple buffer fallback"), true)) {
            uiImIndent();
                uiImCheckbox(__("Enable triple buffer fallback"), tripleFallbackDraft);
                uiImLabel(_("Use when advanced blend is unavailable or causes issues."));
            uiImUnindent();
        }
    }

    void renderRenderingSettingsSection() {
        renderRenderingSettingsContent(false);
    }

    override
    void onUpdate() {
        ensureDraftLoaded();
        vec2 avail = uiImAvailableSpace();
        float lhs = 196;
        float rhs = avail.x-lhs;
        immutable float footerHeight = 38.0f;
        igPushStyleColor(ImGuiCol.ChildBg, ImVec4(1, 1, 1, 0));
        igPushStyleVar(ImGuiStyleVar.ChildBorderSize, 0.0f);
        scope(exit) {
            igPopStyleVar();
            igPopStyleColor();
        }

        if (uiImBeginChild("##LHS", vec2(lhs, -footerHeight), false)) {
            avail = uiImAvailableSpace();
            uiImPush(0);
            if (uiImSelectable(__("Tracking"), selected == SelectedMode.Tracking)) {
                selected = SelectedMode.Tracking;
            }
            if (uiImSelectable(__("Rending"), selected == SelectedMode.Rendering)) {
                selected = SelectedMode.Rendering;
            }
            uiImPop();            
        }
        uiImEndChild();

        uiImSameLine(0, 0);

        if (uiImBeginChild("##RHS", vec2(rhs, -footerHeight), false)) {
            final switch (selected) {
            case SelectedMode.Tracking:
                renderTrackingSettingsSection();
                break;
            case SelectedMode.Rendering:
                renderRenderingSettingsSection();
                break;
            }
        }
        uiImEndChild();

        igSetCursorPosX(max(0.0f, uiImAvailableSpace().x - 72.0f));
        if (uiImButton(__("Apply"), vec2(64, 0))) {
            applySettings();
        }
    }

    override
    void onClose() {
        trackerDraftLoaded = false;
        neSetMeasureFPS(prevMeasureFPS);
    }


    this() {
        super(_("Settings"));
        flags = ImGuiWindowFlags.NoTitleBar | ImGuiWindowFlags.NoResize |
                ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoSavedSettings |
                ImGuiWindowFlags.NoScrollbar;
        prevMeasureFPS = neGetMeasureFPS();
    }

}
