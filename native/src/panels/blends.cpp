#include "factory.hpp"
#include "scene/package.hpp"
#include "nijiui/widgets/basic.hpp"
#include "i18n/tr.hpp"
#include <cstdio>
namespace nijiexpose::panels {
using namespace nijiui;
namespace {
std::string number(float value){char buf[64];std::snprintf(buf,sizeof(buf),"%g",static_cast<double>(value));return buf;}
std::string boneText(const ft::Bone& b){return "Bone(["+number(b.position.x)+", "+number(b.position.y)+", "+number(b.position.z)+"], ["+number(b.rotation.w)+", "+number(b.rotation.x)+", "+number(b.rotation.y)+", "+number(b.rotation.z)+"])";}
}
class BlendshapesPanel : public Panel {
    void onUpdate() override {
        auto space=scene::insScene()->space;
        auto zone=space->currentZone();
        if(!zone)return;
        auto blendshapes=space->getAllBlendshapeNames();
        auto bones=space->getAllBoneNames();
        for(auto& blendshape:blendshapes)uiImLabel(blendshape+" = "+number(zone->getBlendshapeFor(blendshape)));
        for(auto& bone:bones)uiImLabel(bone+" = "+boneText(zone->getBoneFor(bone)));
    }
public:
    BlendshapesPanel():Panel("Blendshapes",i18n::_("Blendshapes"),true){}
};
std::shared_ptr<Panel> createBlendshapesPanel(){return std::make_shared<BlendshapesPanel>();}
}
