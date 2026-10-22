#pragma once
#include "nijiui/panel.hpp"
#include <math/types.hpp>
namespace nijiexpose::panels {
class ScenePanel:public nijiui::Panel {
    nicxlive::core::math::Vec4 clearColor{0,0,0,0};
    void loadBackground(const std::string& file);
    void setBGColor(nicxlive::core::math::Vec4 color);
protected:void onUpdate() override;
public:ScenePanel();void renderSceneSettingsContent();
};
}
