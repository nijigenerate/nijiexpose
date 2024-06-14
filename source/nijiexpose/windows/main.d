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

version(linux) import dportals;

private {
    struct InochiWindowSettings {
        int width;
        int height;
    }

    struct PuppetSavedData {
        float scale;
    }
}

class nijiexposeWindow : InApplicationWindow {
private:
    Adaptor adaptor;
    version (InBranding) Texture logo;

    void loadModels(string[] args) {
        foreach(arg; args) {
            import std.file : exists;
            if (!exists(arg)) continue;
            try {
                insSceneAddPuppet(arg, inLoadPuppet(arg));
            } catch(Exception ex) {
                uiImDialog(__("Error"), "Could not load %s, %s".format(arg, ex.msg));
            }
        }
    }

protected:
    override
    void onEarlyUpdate() {
        insUpdateScene();
        insSendFrame();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        inDrawScene(vec4(0, 0, width, height));
    }

    override
    void onUpdate() {
        if (!inInputIsnijiui()) {
            if (inInputMouseDoubleClicked(MouseButton.Left)) this.showUI = !showUI;
            insInteractWithScene();

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

                    uiImEndMenu();
                }

                if (uiImBeginMenu(__("Tools"))) {

                    // Resets the tracking out range to be in the coordinate space of min..max
                    if (uiImMenuItem(__("Reset Tracking Out"))) {
                        if (insSceneSelectedSceneItem()) {
                            foreach(ref binding; insSceneSelectedSceneItem.bindings) {
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

/*
                // DONATE BUTTON
                avail = uiImAvailableSpace();
                vec2 donateBtnLength = uiImMeasureString(_("Donate")).x+16;
                uiImDummy(vec2(avail.x-donateBtnLength.x, 0));
                if (uiImMenuItem(__("Donate"))) {
                    uiOpenLink("https://www.patreon.com/LunaFoxgirlVT");
                }
*/
            uiImEndMainMenuBar();
        }

        version(linux) dpUpdate();
    }

    override
    void onResized(int w, int h) {
        inSetViewport(w, h);
        inSettingsSet("window", InochiWindowSettings(width, height));
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
        super("nijiexpose %s".format(INS_VERSION), windowSettings.width, windowSettings.height);
        
        // Initialize nijilive
        inInit(&inGetTime);
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