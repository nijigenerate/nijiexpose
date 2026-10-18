#pragma once
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>
#include <limits>
namespace nijiui {
class Panel {
    std::string name_, displayName_;
    bool defaultVisibility;
    std::string windowID;
    bool drewContents = false, wasVisible = false, embeddedMode = false;
protected:
    ImVec2 panelSpace{std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()};
    virtual void onUpdate() = 0;
    ImGuiWindowFlags flags = 0;
    virtual void onBeginUpdate();
    virtual void onEndUpdate();
    virtual void onInit() {}
public:
    virtual ~Panel() = default;
    bool visible;
    bool alwaysVisible = false;
    Panel(std::string name, std::string displayName, bool defaultVisibility);
    void init_();
    const std::string& name() const;
    const std::string& displayName() const;
    const char* displayNameC() const;
    bool isEmbedded() const;
    void update();
    void updateEmbedded();
    bool getDefaultVisibility() const;
};
extern thread_local std::vector<std::shared_ptr<Panel>> inPanels;
extern thread_local bool inPanelsSuspended;
void inAddPanel(std::shared_ptr<Panel> panel);
void inUpdatePanels();
void inSetPanelsSuspended(bool suspended);
void inInitPanels();
}
