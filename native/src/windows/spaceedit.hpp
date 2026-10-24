#pragma once
#include "nijiui/toolwindow.hpp"
#include "tracking/vspace.hpp"
namespace nijiexpose::windows {
class SpaceEditor : public nijiui::ToolWindow {
    std::shared_ptr<tracking::VirtualSpaceZone> editingZone;
    std::string newZoneName;
    std::map<std::shared_ptr<ft::Adaptor>,std::map<std::string,std::string>> options;
    void addZone();
    void switchZone(std::shared_ptr<tracking::VirtualSpaceZone> zone);
    void ensureEditingZone();
    void refreshOptionsList();
    void adaptorDelete(size_t idx);
    void adaptorMenu(size_t idx);
    void zoneMenu(size_t idx);
    void adaptorHint(std::shared_ptr<ft::Adaptor>& source);
    void adaptorSelect(size_t i,std::shared_ptr<ft::Adaptor>& source,const char* adaptorName);
public:
    SpaceEditor();
    void applyChanges();
    void renderEditorSection(bool showApplyButton=false);
    void renderEmbeddedSection();
    void onBeginUpdate() override;
    void onUpdate() override;
};
}
