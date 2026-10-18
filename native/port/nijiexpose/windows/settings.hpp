#pragma once
#include "nijiui/toolwindow.hpp"
#include "nijiexpose/tracking/tracker.hpp"
#include <functional>
namespace nijiexpose::windows {
class SettingWindow : public nijiui::ToolWindow {
    bool prevMeasureFPS;
    std::shared_ptr<tracking::Tracker> trackerDraft;
    bool trackerDraftLoaded=false;
    int throttlingDraft=1;
    bool tripleFallbackDraft=false;
    std::string languageDraft="en";
    std::function<void(int)> setThrottling;
    void copyTrackerState(std::shared_ptr<tracking::Tracker>& dst,const std::shared_ptr<tracking::Tracker>& src);
    void ensureDraftLoaded();
public:
    enum class SelectedMode {General,Tracking,Rendering};
    SelectedMode selected=SelectedMode::General;
    std::optional<std::string> pythonPath;
    bool pythonPathTested=false;
    explicit SettingWindow(std::function<void(int)> setThrottling);
    void applyTrackingSettings();
    void applyRenderingSettings();
    void applyLanguageSettings();
    void applySettings();
    void onBeginUpdate() override;
    void onUpdate() override;
    void onClose() override;
    void renderGeneralSettingsSection();
    void renderLanguageSettingsContent(bool embedded);
    void renderTrackingSettingsContent(bool embedded);
    void renderTrackingSettingsSection();
    void renderRenderingSettingsContent(bool embedded);
    void renderRenderingSettingsSection();
};
}
