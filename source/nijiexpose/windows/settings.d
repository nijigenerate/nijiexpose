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

import std.algorithm.mutation;

class SettingWindow : ToolWindow {
private:
    bool prevMeasureFPS;
public:
    SelectedMode selected = SelectedMode.Tracking;

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

    override
    void onUpdate() {
        vec2 avail = uiImAvailableSpace();
        float lhs = 196;
        float rhs = avail.x-lhs;

        if (uiImBeginChild("##LHS", vec2(lhs, -28), true)) {
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

        if (uiImBeginChild("##RHS", vec2(rhs, -28), true)) {
            avail = uiImAvailableSpace();
            switch (selected) {
            case SelectedMode.Tracking:
                if (uiImHeader(__("Tracking"), true)) {
                    uiImIndent();
                        auto tracker = ngTracker();
                        if (uiImCheckbox(__("Enable tracking"), tracker.enabled)) {
                            inSettingsSet("tracker", tracker);
                        }
                        if (tracker.enabled) {
                            tracker.update();
                            if (uiImBeginCategory("##tracker")) {
                                uiImLabel(_("Camera device"));
                                uiImSameLine();
                                auto deviceList = tracker.listDevices();
                                string currentDeviceName = (tracker.device < deviceList.length)? deviceList[tracker.device].name: _("Select device...");
                                if (igBeginCombo("##device", currentDeviceName.toStringz ,ImGuiComboFlags.None)) {
                                    if (deviceList !is null) {
                                        foreach (device; deviceList) {
                                            if (uiImSelectable(device.name.toStringz, device.id == tracker.device)) {
                                                tracker.device = device.id;
                                                inSettingsSet("tracker", tracker);
                                            }
                                        }
                                    }
                                    igEndCombo();
                                }
                                uiImLabel(_("Tracker executable path"));
                                uiImSameLine();
                                if (uiImInputText("##trackerPath", tracker.trackerPath)) {
                                    inSettingsSet("tracker", tracker);
                                }
                                uiImLabel(_("Host name"));
                                uiImSameLine();
                                if (uiImInputText("##host", tracker.hostname)) {
                                    inSettingsSet("tracker", tracker);
                                }
                                uiImLabel(_("Port number"));
                                uiImSameLine();
                                if (igInputInt("##PortNumber", cast(int*)&tracker.port)) {
                                    inSettingsSet("tracker", tracker);
                                }
                                if (uiImCheckbox(__("Flip input"), tracker.flipped)) {
                                    inSettingsSet("tracker", tracker);
                                }
                                if (uiImCheckbox(__("Show camera tracking window"), tracker.showWindow)) {
                                    inSettingsSet("tracker", tracker);
                                }
                            }
                            uiImEndCategory();
                        }
                    uiImUnindent();
                }
                break;
            case SelectedMode.Rendering:
                if (uiImHeader(__("V-Sync throttling"), true)) {
                    uiImIndent();
                        uiImLabel("%s (%s)".format(_("Throtting interval"), _("Experimental")));

                        int throttling = inSettingsGet!int("throttlingRate", 1);
                        if (igSliderInt("##THROTTLING", &throttling, 0, 6)) {
                            inSettingsSet("throttlingRate", throttling);
                            neWindowSetThrottlingRate(throttling);
                        }
                        uiImSameLine();
                        uiImLabel(_("Frame rate: %.2f fps".format(neGetFPS())));
                    uiImUnindent();
                }
                break;
            default:
            }
        }
        uiImEndChild();

        uiImDummy(vec2(-64, 0));
        uiImSameLine(0, 0);
        if (uiImButton(__("OK"), vec2(64, 0))) {
                import std.stdio;
            if (ngTracker.enabled) {
                ngTracker.restart();
            } else {
                ngTracker.terminate();
            }
            neSetMeasureFPS(prevMeasureFPS);
            this.close();
        }
    }


    this() {
        super(_("Settings"));
        flags = ImGuiWindowFlags.NoTitleBar | ImGuiWindowFlags.NoResize |
                ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoSavedSettings |
                ImGuiWindowFlags.NoScrollbar;
        prevMeasureFPS = neGetMeasureFPS();
    }

}