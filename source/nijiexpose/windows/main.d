/*
    Copyright © 2022, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.windows.main;
import nijiexpose.windows;
import nijiexpose.scene;
import nijiexpose.log;
import nijiexpose.framesend;
import nijiexpose.plugins;
import nijiexpose.io;
import nijiui;
import nijiui.widgets;
import nijiui.toolwindow;
import nijiui.panel;
import nijiui.input;
import nijilive;
import ft;
import i18n;
import nijiui.utils.link;
import std.format;
import nijiexpose.ver;
import bindbc.opengl;
import nijiexpose.windows.utils;
import nijiexpose.io.image;
import std.path;
import std.string;

version(linux) import dportals;

private {
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

class nijiexposeWindow : InApplicationWindow {
private:
    Adaptor adaptor;
    version (InBranding) Texture logo;

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
        inDrawScene(vec4(0, 0, width, height));
    }

    override
    void onUpdate() {
        if (!inInputIsnijiui()) {
            if (inInputMouseDoubleClicked(MouseButton.Left)) this.showUI = !showUI;
            insScene.interact();

            if (getDraggedFiles().length > 0) {
                loadModels(getDraggedFiles());
            }
        }

        if (showUI) {
            uiImBeginMainMenuBar();
                vec2 avail = uiImAvailableSpace();
                version (InBranding) {
                    uiImImage(logo.getTextureId(), vec2(avail.y*2, avail.y*2));
                }

                if (uiImBeginMenu(__("File"))) {

                    if (uiImMenuItem(__("Open"))) {
                        const TFD_Filter[] filters = [
                            { ["*.inp"], "nijilive Puppet (*.inp)" }
                        ];

                        string parentWindow = "";
                        version(linux) {
                            static if (is(typeof(&getWindowHandle))) {
                                parentWindow = getWindowHandle();
                            }
                        }
                        string file = insShowOpenDialog(filters, _("Open..."), parentWindow);
                        if (file) loadModels([file]);
                    }

                    uiImSeperator();

                    if (uiImMenuItem(__("Exit"))) {
                        this.close();
                    }

                    uiImEndMenu();
                }

                if (uiImBeginMenu(__("View"))) {

                    uiImLabelColored(_("Panels"), vec4(0.8, 0.3, 0.3, 1));
                    uiImSeperator();

                    foreach(panel; inPanels) {
                        if (uiImMenuItem(panel.displayNameC, "", panel.visible)) {
                            panel.visible = !panel.visible;
                        }
                    }
                    
                    uiImNewLine();

                    uiImLabelColored(_("Configuration"), vec4(0.8, 0.3, 0.3, 1));
                    uiImSeperator();
                    if (uiImMenuItem(__("Virtual Space"))) {
                        inPushToolWindow(new SpaceEditor());
                    }

                    if (uiImMenuItem(__("Settings"))) {
                        inPushToolWindow(new SettingWindow());
                    }
                    uiImEndMenu();
                }

                if (uiImBeginMenu(__("Tools"))) {

                    // Resets the tracking out range to be in the coordinate space of min..max
                    if (uiImMenuItem(__("Reset Tracking Out"))) {
                        if (insScene.selectedSceneItem()) {
                            foreach(ref binding; insScene.selectedSceneItem().bindings) {
                                binding.outRangeToDefault();
                            }
                        }
                    }
                    uiImEndMenu();
                }

                if (uiImBeginMenu(__("Plugins"))) {

                    uiImLabelColored(_("Plugins"), vec4(0.8, 0.3, 0.3, 1));
                    uiImSeperator();

                    foreach(plugin; insPlugins) {
                        if (uiImMenuItem(plugin.getCName, "", plugin.isEnabled)) {
                            plugin.isEnabled = !plugin.isEnabled;
                            insSavePluginState();
                        }
                    }

                    uiImNewLine();

                    uiImLabelColored(_("Tools"), vec4(0.8, 0.3, 0.3, 1));
                    uiImSeperator();
                    if (uiImMenuItem(__("Rescan Plugins"))) {
                        insEnumeratePlugins();
                    }

                    uiImEndMenu();
                }


                if (uiImBeginMenu(__("Help"))) {
                    if (uiImMenuItem(__("Documentation"))) {
                        uiOpenLink("https://github.com/nijigenerate/nijiexpose/wiki");
                    }
                    if (uiImMenuItem(__("About"))) {
                        uiImDialog(__("nijiexpose"),
                        "nijiexpose %s\n(nijilive %s)\n\nMade with <3\nby seagetch and nijigenerate Contributors.\n\nSpecial thanks to Inochi2D project.".format(INS_VERSION, IN_VERSION), DialogLevel.Info);
                    }
                    
                    uiImEndMenu();
                }

                uiImDummy(vec2(4, 0));
                uiImSeperator();
                uiImDummy(vec2(4, 0));
                uiImLabel(_("Double-click to show/hide UI"));

            uiImEndMainMenuBar();
        }

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
        inSetViewport(windowSettings.width, windowSettings.height);

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