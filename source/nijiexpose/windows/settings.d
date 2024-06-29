module nijiexpose.windows.settings;

import nijiexpose.log;
import nijiexpose.scene;
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

    override
    void onUpdate() {
        vec2 avail = uiImAvailableSpace();
        float lhs = 196;
        float rhs = avail.x-lhs;

        int selected = 0;
        if (uiImBeginChild("##LHS", vec2(lhs, -28), true)) {
            avail = uiImAvailableSpace();
            uiImPush(0);
            if (uiImSelectable(__("Rending"), selected == 0)) {
                selected = 0;
            }
            uiImPop();            
        }
        uiImEndChild();

        uiImSameLine(0, 0);

        if (uiImBeginChild("##RHS", vec2(rhs, -28), true)) {
            avail = uiImAvailableSpace();
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
        }
        uiImEndChild();

        uiImDummy(vec2(-64, 0));
        uiImSameLine(0, 0);
        if (uiImButton(__("OK"), vec2(64, 0))) {
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