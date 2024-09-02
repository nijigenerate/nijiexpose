/*
    Copyright © 2022, Inochi2D Project
    Copyright © 2024, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.panels.plugin;
import nijiexpose.tracking.expr;
import nijiexpose.plugins;
import nijiexpose.plugins.api;
import nijiui.panel;
import i18n;
import nijiexpose.scene;
import nijiui;
import nijiui.widgets;
import nijiexpose.log;
import inmath;
import std.format;

class PluginPanel : Panel {
private:

protected:

    override 
    void onUpdate() {
    
        foreach(ref plugin; insPlugins) {
            uiImPush(&plugin);
                if (plugin.isEnabled && !plugin.hasError) {
                    if (uiImHeader(plugin.getCName(), true)) {
                        uiImPushTextWrapPos();
                            uiImIndent();
                                if (plugin.hasError) {
                                    uiImLabelColored(
                                        _("%s has crashed, options are disabled.").format(plugin.getInfo().pluginName), 
                                        vec4(1, 0.3, 0.3, 1)
                                    );
                                } else if (plugin.hasEvent("onRenderUI")) {
                                        insPluginBegnijiui();
                                        try {
                                            plugin.callEvent("onRenderUI");
                                        } catch(Exception ex) {
                                            insLogErr(_("%s (plugin): %s"), plugin.getInfo().pluginId, ex.msg);
                                        }
                                        insPluginEndUI();
                                } else {
                                    uiImLabel(_("Plugin cannot be configured."));
                                }
                            uiImUnindent();
                        uiImPopTextWrapPos();
                    }
                }
            uiImPop();
        }
    }

public:
    this() {
        super("Plugins", _("Plugins"), true);
    }
}

mixin inPanel!PluginPanel;
