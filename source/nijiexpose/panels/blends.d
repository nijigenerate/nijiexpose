module nijiexpose.panels.blends;
import nijiexpose.tracking.expr;
import inui.panel;
import i18n;
import nijiexpose.scene;
import nijiexpose.tracking;
import inui;
import inui.widgets;
import nijiexpose.log;
import inmath;
import std.string;
import std.uni;
import std.algorithm.searching;
import ft;


class BlendshapesPanel : Panel {

protected:

    override 
    void onUpdate() {
        auto blendshapes = insScene.space.getAllBlendshapeNames();
        auto bones = insScene.space.getAllBoneNames();
        foreach(blendshape; blendshapes) {
            uiImLabel("%s = %s".format(blendshape, insScene.space.currentZone.getBlendshapeFor(blendshape)));
        }
        foreach(bone; bones) {
            uiImLabel("%s = %s".format(bone, insScene.space.currentZone.getBoneFor(bone)));
        }
    }

public:
    this() {
        super("Blendshapes", _("Blendshapes"), true);
    }
}

mixin inPanel!BlendshapesPanel;