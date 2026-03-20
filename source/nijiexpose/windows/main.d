/*
    Copyright © 2022, Inochi2D Project
    Copyright © 2024, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.windows.main;
import nijiexpose.windows;
import nijiexpose.windows.utils;
import nijiexpose.scene;
import nijiexpose.log;
import nijiexpose.framesend;
import nijiexpose.plugins;
import nijiexpose.io;
import nijiexpose.io.image;
import nijiexpose.tracking.tracker;
import nijiui;
import nijiui.widgets;
import nijiui.toolwindow;
import nijiui.panel;
import nijiui.input;
import nijilive;
import nijilive.core.render.backends.opengl.runtime : oglDrawScene;
import ft;
import i18n;
import nijiui.utils.link;
import nijiui.core.settings : inSettingsGet, inSettingsSet;
import std.format;
import nijiexpose.ver;
import bindbc.opengl;
import bindbc.imgui;
import std.algorithm.comparison : min, max;
import std.path;
import std.string;

version(linux) import dportals;

private {
    enum NavSurfaceMode {
        DesktopRail,
        CompactBar,
    }

    enum ActivePanelId {
        Scene,
        Tracking,
        Animations,
        Blendshapes,
        Plugins,
        Settings,
        VirtualSpace,
    }

    struct NavItem {
        ActivePanelId id;
        string panelName;
        string icon;
        string label;
    }

    immutable NavItem[] NAV_ITEMS = [
        NavItem(ActivePanelId.Tracking, "Tracking", "\ue429", "Parameters"),
        NavItem(ActivePanelId.Scene, "Scene Settings", "\ue8f4", "View"),
        NavItem(ActivePanelId.Animations, "Animations", "\ue037", "Animations"),
        NavItem(ActivePanelId.Blendshapes, "Blendshapes", "\ue3b4", "Blends"),
        NavItem(ActivePanelId.Plugins, "Plugins", "\ue87b", "Plugins"),
        NavItem(ActivePanelId.Settings, "", "\ue8b8", "Settings"),
        NavItem(ActivePanelId.VirtualSpace, "", "\ue8d4", "Virtual Space"),
    ];

    enum vec4 RAIL_BG = vec4(0.97f, 0.98f, 0.99f, 0.84f);
    enum vec4 RAIL_BORDER = vec4(0.12f, 0.16f, 0.23f, 0.10f);
    enum vec4 OVERLAY_BG = vec4(0.97f, 0.98f, 0.99f, 0.62f);
    enum vec4 OVERLAY_BORDER = vec4(0.12f, 0.16f, 0.23f, 0.10f);
    enum vec4 ACCENT = vec4(0.70f, 0.25f, 0.00f, 1.00f);
    enum vec4 ACCENT_SOFT = vec4(0.70f, 0.25f, 0.00f, 0.14f);
    enum vec4 SHADOW_NEAR = vec4(0.12f, 0.16f, 0.23f, 0.10f);
    enum vec4 SHADOW_FAR = vec4(0.12f, 0.16f, 0.23f, 0.03f);
    enum float RAIL_COLLAPSED_WIDTH = 62.0f;
    enum float RAIL_EXPANDED_WIDTH = 210.0f;
    enum float OUTER_GAP = 18.0f;
    enum float RAIL_TOP = 18.0f;
    enum float RAIL_BOTTOM = 18.0f;

    struct InochiWindowSettings {
        int width;
        int height;
    }

    struct PuppetSavedData {
        float scale;
    }
    nijiexposeWindow window_ = null;
}

nijiexposeWindow neCreateWindow(string[] args) {
    if (!window_) {
        window_ = new nijiexposeWindow(args);
    }
    return window_;
}

void neWindowSetThrottlingRate(int rate) {
    if (window_) {
        window_.setThrottlingRate(rate);
    }
}

private __gshared rect gTrashTargetRect;

void neGetTrashTargetRect(out float x, out float y, out float w, out float h) {
    x = gTrashTargetRect.x;
    y = gTrashTargetRect.y;
    w = gTrashTargetRect.width;
    h = gTrashTargetRect.height;
}

class nijiexposeWindow : InApplicationWindow {
private:
    Adaptor adaptor;
    version (InBranding) Texture logo;
    SettingWindow settingWindow;
    SpaceEditor spaceEditor;
    ActivePanelId activePanel = ActivePanelId.Scene;
    bool navExpanded = false;
    bool overlayOpen = true;
    bool navFaded = false;
    vec2 lastPointerPos;
    double lastNavInteractionAt = 0;

    ActivePanelId sanitizeActivePanel(int rawValue) {
        if (rawValue < cast(int)ActivePanelId.Scene || rawValue > cast(int)ActivePanelId.VirtualSpace) {
            return ActivePanelId.Scene;
        }
        return cast(ActivePanelId)rawValue;
    }

    Panel panelFor(ActivePanelId id) {
        foreach(item; NAV_ITEMS) {
            if (item.id != id) continue;
            if (item.panelName.length == 0) return null;
            foreach(panel; inPanels) {
                if (panel.name() == item.panelName) return panel;
            }
        }
        return null;
    }

    ToolWindow toolWindowFor(ActivePanelId id) {
        final switch(id) {
        case ActivePanelId.Settings:
            return settingWindow;
        case ActivePanelId.VirtualSpace:
            return spaceEditor;
        case ActivePanelId.Scene:
        case ActivePanelId.Tracking:
        case ActivePanelId.Animations:
        case ActivePanelId.Blendshapes:
        case ActivePanelId.Plugins:
            return null;
        }
    }

    NavSurfaceMode navSurfaceMode() {
        if (width <= 1100 || height <= 760) return NavSurfaceMode.CompactBar;
        return NavSurfaceMode.DesktopRail;
    }

    void touchNav() {
        lastNavInteractionAt = inGetTime();
        navFaded = false;
    }

    void updateNavFadeState() {
        vec2 mousePos = inInputMousePosition();
        bool pointerMoved = mousePos.x != lastPointerPos.x || mousePos.y != lastPointerPos.y;
        bool interacted = pointerMoved
            || inInputMouseClicked(MouseButton.Left)
            || inInputMouseClicked(MouseButton.Right)
            || inInputMouseClicked(MouseButton.Middle)
            || inInputMouseScrollDelta() != 0;

        if (interacted || overlayOpen || navExpanded) {
            touchNav();
        } else if ((inGetTime() - lastNavInteractionAt) > 2.6) {
            navFaded = true;
        }

        lastPointerPos = mousePos;
    }

    void syncPanelVisibility() {
        foreach(panel; inPanels) {
            panel.visible = false;
        }
    }

    vec4 withAlpha(vec4 color, float alphaScale) {
        return vec4(color.x, color.y, color.z, color.w * alphaScale);
    }

    float navVisualAlpha() {
        return navFaded && !overlayOpen && !navExpanded ? 0.24f : 1.0f;
    }

    void togglePanel(ActivePanelId id) {
        if (overlayOpen && activePanel == id) {
            overlayOpen = false;
        } else {
            activePanel = id;
            overlayOpen = true;
        }
        inSettingsSet("ui.activePanel", cast(int)activePanel);
        inSettingsSet("ui.overlayOpen", overlayOpen);
    }

    bool usesParameterOverlay(ActivePanelId id) {
        return id == ActivePanelId.Tracking;
    }

    bool drawNavEntry(string id, string icon, string label, bool selected, bool compact) {
        ImVec2 pos;
        igGetCursorScreenPos(&pos);

        float width = compact ? 44.0f : (navExpanded ? 172.0f : 44.0f);
        float height = 44.0f;
        bool clicked = igInvisibleButton(id.toStringz, ImVec2(width, height));
        bool hovered = igIsItemHovered();

        auto drawList = igGetWindowDrawList();
        ImVec2 minPos = pos;
        ImVec2 maxPos = ImVec2(pos.x + width, pos.y + height);
        float visualAlpha = navVisualAlpha();
        if (selected || hovered) {
            vec4 bg = selected ? ACCENT_SOFT : vec4(0.12f, 0.16f, 0.23f, 0.05f);
            bg = withAlpha(bg, visualAlpha);
            ImDrawList_AddRectFilled(drawList, minPos, maxPos, igColorConvertFloat4ToU32(ImVec4(bg.x, bg.y, bg.z, bg.w)), 16.0f);
        }

        vec4 iconColor = selected ? ACCENT : vec4(0.70f, 0.25f, 0.00f, 0.90f);
        iconColor = withAlpha(iconColor, visualAlpha);
        ImVec2 iconSize;
        igCalcTextSize(&iconSize, icon.toStringz);
        ImVec2 iconPos = ImVec2(minPos.x + 22.0f - (iconSize.x * 0.5f), minPos.y + ((height - iconSize.y) * 0.5f));
        ImDrawList_AddText(drawList, iconPos, igColorConvertFloat4ToU32(ImVec4(iconColor.x, iconColor.y, iconColor.z, iconColor.w)), icon.toStringz);

        if (!compact && navExpanded) {
            vec4 labelColor = selected ? vec4(0.45f, 0.18f, 0.00f, 1.00f) : vec4(0.12f, 0.16f, 0.23f, 0.90f);
            labelColor = withAlpha(labelColor, visualAlpha);
            ImVec2 labelSize;
            auto translated = _(label);
            igCalcTextSize(&labelSize, translated.toStringz);
            ImVec2 labelPos = ImVec2(minPos.x + 48.0f, minPos.y + ((height - labelSize.y) * 0.5f));
            ImDrawList_AddText(drawList, labelPos, igColorConvertFloat4ToU32(ImVec4(labelColor.x, labelColor.y, labelColor.z, labelColor.w)), translated.toStringz);
        }

        if (hovered) {
            uiImTooltip(_(label));
        }
        return clicked;
    }

    bool drawIconOnlyEntry(string id, string icon, ImVec2 pos, float size, vec4 iconColor, bool hoveredBg) {
        igSetCursorScreenPos(pos);
        bool clicked = igInvisibleButton(id.toStringz, ImVec2(size, size));
        bool hovered = igIsItemHovered();
        float visualAlpha = navVisualAlpha();
        auto drawList = igGetWindowDrawList();
        if (hovered && hoveredBg) {
            vec4 bgColor = withAlpha(vec4(0.12f, 0.16f, 0.23f, 0.05f), visualAlpha);
            ImDrawList_AddRectFilled(
                drawList,
                pos,
                ImVec2(pos.x + size, pos.y + size),
                igColorConvertFloat4ToU32(ImVec4(bgColor.x, bgColor.y, bgColor.z, bgColor.w)),
                10.0f
            );
        }
        iconColor = withAlpha(iconColor, visualAlpha);
        ImVec2 iconSize;
        igCalcTextSize(&iconSize, icon.toStringz);
        ImVec2 iconPos = ImVec2(pos.x + ((size - iconSize.x) * 0.5f), pos.y + ((size - iconSize.y) * 0.5f));
        ImDrawList_AddText(drawList, iconPos, igColorConvertFloat4ToU32(ImVec4(iconColor.x, iconColor.y, iconColor.z, iconColor.w)), icon.toStringz);
        return clicked;
    }

    void drawRailButton(NavItem item, bool compact) {
        if (drawNavEntry("nav_" ~ item.label, item.icon, item.label, overlayOpen && activePanel == item.id, compact)) {
            togglePanel(item.id);
        }
    }

    void drawUtilityButton(string icon, string label, void delegate() action, bool compact) {
        if (drawNavEntry("utility_" ~ label, icon, label, false, compact)) {
            action();
        }
    }

    void drawTrashTarget(bool compact) {
        immutable float size = 44.0f;
        if (!compact) {
            ImVec2 winPos;
            igGetWindowPos(&winPos);
            ImVec2 winSize;
            igGetWindowSize(&winSize);
            immutable float slotX = winPos.x + 10.0f;
            immutable float slotY = winPos.y + winSize.y - size - 10.0f;
            immutable float slotW = winSize.x - 20.0f;
            immutable float slotH = size;
            ImVec2 pos = ImVec2(slotX + ((slotW - size) * 0.5f), slotY);
            drawIconOnlyEntry("trash_target", "\ue872", pos, size, vec4(0.70f, 0.25f, 0.00f, 0.90f), true);
            gTrashTargetRect = rect(slotX, slotY, slotW, slotH);
        } else {
            ImVec2 pos;
            igGetCursorScreenPos(&pos);
            drawIconOnlyEntry("trash_target", "\ue872", pos, size, vec4(0.70f, 0.25f, 0.00f, 0.90f), true);
            gTrashTargetRect = rect(pos.x, pos.y, size, size);
        }
    }

    void drawNavigationShell() {
        if (!showUI) return;
        auto compact = navSurfaceMode() == NavSurfaceMode.CompactBar;
        ImGuiWindowFlags flags = ImGuiWindowFlags.NoDecoration
            | ImGuiWindowFlags.NoMove
            | ImGuiWindowFlags.NoResize
            | ImGuiWindowFlags.NoSavedSettings
            | ImGuiWindowFlags.NoNavFocus;
        if (compact) {
            flags |= ImGuiWindowFlags.AlwaysAutoResize;
        }

        float visualAlpha = navVisualAlpha();
        float railAlpha = navFaded && !overlayOpen && !navExpanded ? 0.22f : RAIL_BG.w;
        float borderAlpha = navFaded && !overlayOpen && !navExpanded ? 0.06f : RAIL_BORDER.w;
        float shadowNearAlpha = SHADOW_NEAR.w * visualAlpha;
        float shadowFarAlpha = SHADOW_FAR.w * visualAlpha;

        float railX = OUTER_GAP;
        float railY = RAIL_TOP;
        float railW = compact ? 0.0f : (navExpanded ? RAIL_EXPANDED_WIDTH : RAIL_COLLAPSED_WIDTH);
        float railH = compact ? 0.0f : cast(float)height - (RAIL_TOP + RAIL_BOTTOM);

        if (!compact) {
            auto bgDrawList = igGetBackgroundDrawList_Nil();
            ImDrawList_AddRectFilled(
                bgDrawList,
                ImVec2(railX + 6.0f, railY + 8.0f),
                ImVec2(railX + railW + 6.0f, railY + railH + 8.0f),
                igColorConvertFloat4ToU32(ImVec4(SHADOW_FAR.x, SHADOW_FAR.y, SHADOW_FAR.z, shadowFarAlpha)),
                24.0f
            );
            ImDrawList_AddRectFilled(
                bgDrawList,
                ImVec2(railX + 2.0f, railY + 3.0f),
                ImVec2(railX + railW + 2.0f, railY + railH + 3.0f),
                igColorConvertFloat4ToU32(ImVec4(SHADOW_NEAR.x, SHADOW_NEAR.y, SHADOW_NEAR.z, shadowNearAlpha)),
                24.0f
            );
        }

        igPushStyleColor(ImGuiCol.WindowBg, ImVec4(RAIL_BG.x, RAIL_BG.y, RAIL_BG.z, railAlpha));
        igPushStyleColor(ImGuiCol.Border, ImVec4(RAIL_BORDER.x, RAIL_BORDER.y, RAIL_BORDER.z, borderAlpha));
        igPushStyleVar(ImGuiStyleVar.WindowBorderSize, 1.0f);
        igPushStyleVar(ImGuiStyleVar.WindowPadding, compact ? ImVec2(10, 10) : ImVec2(10, 10));
        igPushStyleVar(ImGuiStyleVar.WindowRounding, 24.0f);
        scope(exit) {
            igPopStyleVar(3);
            igPopStyleColor(2);
        }

        igSetNextWindowBgAlpha(railAlpha);
        igSetNextWindowPos(ImVec2(railX, railY), ImGuiCond.Always, ImVec2(0, 0));
        igSetNextWindowSize(compact ? ImVec2(0, 0) : ImVec2(railW, railH), ImGuiCond.Always);

        if (igBegin("nijikan_shell_nav###nijikan_shell_nav", null, flags)) {
            if (compact) {
                if (drawNavEntry("menu_toggle", "\ue5d2", "Menu", false, true)) {
                    navExpanded = false;
                }
                uiImSameLine();
            } else {
                if (drawNavEntry("menu_toggle", "\ue5d2", "Menu", false, false)) {
                    navExpanded = !navExpanded;
                }
            }

            if (compact) uiImSameLine();
            drawUtilityButton("\ue2c8", "Open", {
                const TFD_Filter[] filters = [{ ["*.inp"], "nijilive Puppet (*.inp)" }];
                string parentWindow = "";
                version(linux) {
                    static if (is(typeof(&getWindowHandle))) {
                        parentWindow = getWindowHandle();
                    }
                }
                string file = insShowOpenDialog(filters, _("Open..."), parentWindow);
                if (file) loadModels([file]);
            }, compact);

            foreach(index, item; NAV_ITEMS) {
                if (compact) uiImSameLine();
                drawRailButton(item, compact);
            }

            if (compact) {
                uiImSameLine();
                drawTrashTarget(true);
            } else {
                drawTrashTarget(false);
            }
        }
        igEnd();
    }

    void drawOverlayHost() {
        if (!showUI || !overlayOpen) return;
        Panel active = panelFor(activePanel);
        ToolWindow activeWindow = toolWindowFor(activePanel);
        if (active is null && activeWindow is null) return;

        auto compact = navSurfaceMode() == NavSurfaceMode.CompactBar;
        ImGuiWindowFlags flags = ImGuiWindowFlags.NoCollapse
            | ImGuiWindowFlags.NoTitleBar
            | ImGuiWindowFlags.NoSavedSettings
            | ImGuiWindowFlags.NoNavFocus;

        bool parameterOverlay = usesParameterOverlay(activePanel);
        immutable float compactBarHeight = 64.0f;
        immutable float compactOverlayGap = 10.0f;
        float overlayW;
        float overlayH;
        float overlayX;
        float overlayY;
        if (parameterOverlay && !compact) {
            overlayW = min(max(cast(float)width * 0.22f, 280.0f), 360.0f);
            overlayH = cast(float)height - (RAIL_TOP + RAIL_BOTTOM);
            overlayX = OUTER_GAP + (navExpanded ? RAIL_EXPANDED_WIDTH : RAIL_COLLAPSED_WIDTH) + 10.0f;
            overlayY = RAIL_TOP;
        } else if (parameterOverlay && compact) {
            overlayW = min(cast(float)width - 28.0f, 320.0f);
            overlayX = OUTER_GAP;
            overlayY = RAIL_TOP + compactBarHeight + compactOverlayGap;
            overlayH = cast(float)height - overlayY - OUTER_GAP;
        } else {
            overlayW = compact
                ? min(cast(float)width - 32.0f, 520.0f)
                : min(cast(float)width * 0.62f, 760.0f);
            overlayH = compact
                ? min(cast(float)height - 120.0f, 620.0f)
                : min(cast(float)height * 0.76f, 760.0f);
            overlayX = (cast(float)width - overlayW) * 0.5f;
            overlayY = (cast(float)height - overlayH) * 0.5f;
        }

        {
            auto bgDrawList = igGetBackgroundDrawList_Nil();
            ImVec2 shadowMin = ImVec2(overlayX, overlayY);
            ImVec2 shadowMax = ImVec2(overlayX + overlayW, overlayY + overlayH);
            ImDrawList_AddRectFilled(
                bgDrawList,
                ImVec2(shadowMin.x + 6.0f, shadowMin.y + 8.0f),
                ImVec2(shadowMax.x + 6.0f, shadowMax.y + 8.0f),
                igColorConvertFloat4ToU32(ImVec4(SHADOW_FAR.x, SHADOW_FAR.y, SHADOW_FAR.z, SHADOW_FAR.w)),
                18.0f
            );
            ImDrawList_AddRectFilled(
                bgDrawList,
                ImVec2(shadowMin.x + 2.0f, shadowMin.y + 3.0f),
                ImVec2(shadowMax.x + 2.0f, shadowMax.y + 3.0f),
                igColorConvertFloat4ToU32(ImVec4(SHADOW_NEAR.x, SHADOW_NEAR.y, SHADOW_NEAR.z, SHADOW_NEAR.w)),
                15.0f
            );
        }

        igPushStyleColor(ImGuiCol.WindowBg, ImVec4(OVERLAY_BG.x, OVERLAY_BG.y, OVERLAY_BG.z, OVERLAY_BG.w));
        igPushStyleColor(ImGuiCol.Border, ImVec4(OVERLAY_BORDER.x, OVERLAY_BORDER.y, OVERLAY_BORDER.z, OVERLAY_BORDER.w));
        igPushStyleVar(ImGuiStyleVar.WindowBorderSize, 1.0f);
        igPushStyleVar(ImGuiStyleVar.WindowPadding, ImVec2(16, 14));
        igPushStyleVar(ImGuiStyleVar.WindowRounding, 13.0f);
        scope(exit) {
            igPopStyleVar(3);
            igPopStyleColor(2);
        }

        igSetNextWindowPos(ImVec2(overlayX, overlayY), ImGuiCond.Always, ImVec2(0, 0));
        igSetNextWindowSize(ImVec2(overlayW, overlayH), ImGuiCond.Always);

        string title = active !is null ? active.displayName() : activeWindow.name();
        string windowTitle = "%s###nijikan_overlay_host".format(title);

        if (!parameterOverlay && igIsMouseClicked(ImGuiMouseButton.Left)) {
            ImVec2 mousePos;
            igGetMousePos(&mousePos);
            auto overlayRect = ImRect_ImRect(overlayX, overlayY, overlayX + overlayW, overlayY + overlayH);
            scope(exit) ImRect_destroy(overlayRect);
            if (!ImRect_Contains(overlayRect, mousePos)) {
                overlayOpen = false;
                inSettingsSet("ui.overlayOpen", overlayOpen);
                return;
            }
        }

        if (igBegin(windowTitle.toStringz, null, flags)) {
            igPushStyleVar(ImGuiStyleVar.FramePadding, ImVec2(8, 6));
            scope(exit) igPopStyleVar();

            ImVec2 headerStart;
            igGetCursorScreenPos(&headerStart);
            uiImLabelColored(title, ACCENT);
            ImVec2 closePos = ImVec2(overlayX + overlayW - 44.0f, headerStart.y);
            if (drawIconOnlyEntry("overlay_close", "\ue5cd", closePos, 28.0f, vec4(0.12f, 0.16f, 0.23f, 0.90f), true)) {
                overlayOpen = false;
                inSettingsSet("ui.overlayOpen", overlayOpen);
            }
            igSetCursorScreenPos(ImVec2(headerStart.x, headerStart.y + 28.0f));

            uiImSeperator();
            if (uiImBeginChild("nijikan_overlay_body###nijikan_overlay_body", vec2(0, 0), false)) {
                if (active !is null) {
                    active.updateEmbedded();
                } else {
                    activeWindow.updateEmbedded();
                }
            }
            uiImEndChild();
        }
        igEnd();
    }

    void loadModels(string[] args) {
        foreach(arg; args) {
            string filebase = arg.baseName;

            switch(filebase.extension.toLower) {                
                case ".png", ".tga", ".jpeg", ".jpg":
                    insScene.addPuppet(arg, neLoadModelFromImage(arg));
                    break;

                case ".inp", ".inx":
                    import std.file : exists;
                    if (!exists(arg)) continue;
                    try {
                        insScene.addPuppet(arg, inLoadPuppet(arg));
                    } catch(Exception ex) {
                        uiImDialog(__("Error"), "Could not load %s, %s".format(arg, ex.msg));
                    }
                    break;
                default:
                    uiImDialog(__("Error"), _("Could not load %s, unsupported file format.").format(arg));
                    break;
            }
        }
    }

protected:
    override
    void onEarlyUpdate() {
        insScene.update();
        insSendFrame();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        oglDrawScene(vec4(0, 0, width, height));
    }

    override
    void onUpdate() {
        syncPanelVisibility();
        updateNavFadeState();
        if (!inInputIsnijiui()) {
            if (inInputMouseDoubleClicked(MouseButton.Left)) this.showUI = !showUI;
            insScene.interact();

            if (getDraggedFiles().length > 0) {
                loadModels(getDraggedFiles());
            }
        }

        drawNavigationShell();
        drawOverlayHost();

        version(linux) dpUpdate();
    }

    override
    void onResized(int w, int h) {
        inSetViewport(w, h);
        inSettingsSet("window", InochiWindowSettings(width, height));
        super.onResized(w, h);
    }

    override
    void onClosed() {
    }
public:

    /**
        Construct nijiexpose
    */
    this(string[] args) {
        InochiWindowSettings windowSettings = 
            inSettingsGet!InochiWindowSettings("window", InochiWindowSettings(1024, 1024));

        import nijiexpose.ver;

        int throttlingRate = inSettingsGet!(int)("throttlingRate", 1);

        super("nijiexpose %s".format(INS_VERSION), windowSettings.width, windowSettings.height, throttlingRate);
        
        // Initialize nijilive
        inInit(&inGetTime);
        neSetStyle();
        inSetPanelsSuspended(true);
        inSetViewport(windowSettings.width, windowSettings.height);
        settingWindow = new SettingWindow();
        spaceEditor = new SpaceEditor();
        lastPointerPos = inInputMousePosition();
        lastNavInteractionAt = inGetTime();
        activePanel = sanitizeActivePanel(inSettingsGet!(int)("ui.activePanel", cast(int)ActivePanelId.Scene));
        overlayOpen = inSettingsGet!bool("ui.overlayOpen", true);

        // Preload any specified models
        loadModels(args);

        // uiImDialog(
        //     __("nijiexpose"), 
        //     _("THIS IS BETA SOFTWARE\n\nThis software is incomplete, please lower your expectations."), 
        //     DialogLevel.Warning
        // );

        inGetCamera().scale = vec2(0.5);

        version (InBranding) {
            logo = new Texture(ShallowTexture(cast(ubyte[])import("tex/logo.png")));
            auto tex = ShallowTexture(cast(ubyte[])import("icon_x256.png"));
            setIcon(tex);
        }

        version(linux) dpInit();
    }
}
