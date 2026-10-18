#pragma once
#include "nijiui/panel.hpp"
namespace nijiexpose::panels {
class PluginPanel:public nijiui::Panel {
protected:void onUpdate() override;
public:PluginPanel();
};
}
