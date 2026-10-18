// nijiui/panel.d. Copyright 2022 Inochi2D, 2024 nijigenerate. BSD-2-Clause.
#include "nijiui/panel.hpp"
#include "nijiui/core/settings.hpp"
#include "i18n/tr.hpp"
#include "dcompat/scope.hpp"
namespace nijiui {
thread_local std::vector<std::shared_ptr<Panel>> inPanels;
thread_local bool inPanelsSuspended = false;
void Panel::onBeginUpdate() {
    flags |= ImGuiWindowFlags_NoCollapse;
    drewContents = ImGui::Begin(windowID.c_str(), &visible, flags);
    if (wasVisible != visible) inSettingsSet(name() + ".visible", visible);
    if (!drewContents) return;
    panelSpace = ImGui::GetContentRegionAvail();
}
void Panel::onEndUpdate() {
    ImGui::End();
    wasVisible = visible;
}
Panel::Panel(std::string name, std::string displayName, bool defaultVisibility)
    : name_(std::move(name)), displayName_(std::move(displayName)), defaultVisibility(defaultVisibility), visible(defaultVisibility) {}
void Panel::init_() {
    onInit();
    displayName_ = i18n::_(displayName_);
    if (inSettingsCanGet(name_ + ".visible")) {
        visible = inSettingsGet<bool>(name_ + ".visible");
        wasVisible = visible;
    }
    windowID = displayName_ + "###" + name_;
}
const std::string& Panel::name() const { return name_; }
const std::string& Panel::displayName() const { return displayName_; }
const char* Panel::displayNameC() const { return displayName_.c_str(); }
bool Panel::isEmbedded() const { return embeddedMode; }
void Panel::update() {
    onBeginUpdate();
    if (drewContents) onUpdate();
    onEndUpdate();
}
void Panel::updateEmbedded() {
    embeddedMode = true;
    nijiexpose::dcompat::ScopeExit restore([&] { embeddedMode = false; });
    panelSpace = ImGui::GetContentRegionAvail();
    onUpdate();
}
bool Panel::getDefaultVisibility() const { return defaultVisibility; }
void inAddPanel(std::shared_ptr<Panel> panel) { inPanels.push_back(std::move(panel)); }
void inUpdatePanels() {
    if (inPanelsSuspended) return;
    const auto panels = inPanels;
    for (auto& panel : panels) {
        if (!panel->visible && !panel->alwaysVisible) continue;
        panel->update();
    }
}
void inSetPanelsSuspended(bool suspended) { inPanelsSuspended = suspended; }
void inInitPanels() {
    const auto panels = inPanels;
    for (auto& panel : panels) panel->init_();
}
}
