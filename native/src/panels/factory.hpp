#pragma once
#include "nijiui/panel.hpp"
namespace nijiexpose::panels {
std::shared_ptr<nijiui::Panel> createAnimationsPanel();
std::shared_ptr<nijiui::Panel> createTrackingPanel();
std::shared_ptr<nijiui::Panel> createBlendshapesPanel();
}
