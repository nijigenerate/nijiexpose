#include "settings.hpp"
#include "nijiexpose/i18n.hpp"
#include "nijiexpose/scene/package.hpp"
#include "nijilive/opengl/blending.hpp"
#include "nijiui/core/settings.hpp"
#include "nijiui/widgets/input.hpp"
#include "nijiui/widgets/category.hpp"
#include "nijiui/widgets/dialog.hpp"
#include "i18n/tr.hpp"
#include <algorithm>
#include <filesystem>
#include <cstdio>
namespace nijiexpose::windows {
using namespace nijiui;
using namespace tracking;
using namespace scene;
using namespace ogl;
using i18n::_; using i18n::__;
namespace {
struct ScopeExit { std::function<void()> fn; ~ScopeExit(){fn();} };
std::string joinLines(const std::vector<std::string>& lines){std::string s;for(size_t i=0;i<lines.size();++i){if(i)s+='\n';s+=lines[i];}return s;}
std::string fpsText(){char buf[128];std::snprintf(buf,sizeof(buf),"Frame rate: %.2f fps",neGetFPS());return buf;}
}
void SettingWindow::copyTrackerState(std::shared_ptr<Tracker>& dst, const std::shared_ptr<Tracker>& src) {
        dst->enabled = src->enabled;
        dst->flipped = src->flipped;
        dst->showWindow = src->showWindow;
        dst->keyboard = src->keyboard;
        dst->device = src->device;
        dst->width = src->width;
        dst->height = src->height;
        dst->fps = src->fps;
    }

void SettingWindow::ensureDraftLoaded() {
        if (trackerDraftLoaded) return;
        auto tracker = neTracker();
        if (!trackerDraft) {
            trackerDraft = std::make_shared<Tracker>();
        }
        copyTrackerState(trackerDraft, tracker);
        throttlingDraft = inSettingsGet<int>("throttlingRate", 1);
        tripleFallbackDraft = inSettingsGet<bool>("TripleBufferFallback", nlIsTripleBufferFallbackEnabled());
        languageDraft = neLocaleCurrentCode();
        pythonPathTested = false;
        trackerDraftLoaded = true;
    }


void SettingWindow::applyTrackingSettings() {
        ensureDraftLoaded();
        if (trackerDraft->enabled) {
            auto problem = trackerDraft->validationError();
            if (!problem.empty()) {
                uiImDialog(__("Error"), problem);
                return;
            }
        }
        auto tracker = neTracker();
        copyTrackerState(tracker, trackerDraft);
        inSettingsSetJson("tracker", tracker->serialize());
        inSettingsSave();
        if (tracker->enabled) {
            tracker->restart();
            if (!tracker->launchError.empty()) uiImDialog(__("Error"), tracker->launchError);
            tracker->setupVSpace();
        } else {
            tracker->terminate();
        }
    }

void SettingWindow::applyRenderingSettings() {
        ensureDraftLoaded();
        inSettingsSet("throttlingRate", throttlingDraft);
        setThrottling(throttlingDraft);
        inSettingsSet("TripleBufferFallback", tripleFallbackDraft);
        nlSetTripleBufferFallback(tripleFallbackDraft);
        inSettingsSave();
    }

void SettingWindow::applyLanguageSettings() {
        ensureDraftLoaded();
        neLocaleSet(languageDraft);
        inSettingsSave();
    }

void SettingWindow::applySettings() {
        applyLanguageSettings();
        applyTrackingSettings();
        applyRenderingSettings();
    }
void SettingWindow::onBeginUpdate() {
        neSetMeasureFPS(true);
        ImVec2 wpos = ImVec2(
            ImGui::GetMainViewport()->Pos.x+(ImGui::GetMainViewport()->Size.x/2),
            ImGui::GetMainViewport()->Pos.y+(ImGui::GetMainViewport()->Size.y/2)
        );

        ImVec2 uiSize = ImVec2(
            800, 
            600
        );

        ImGui::SetNextWindowPos(wpos, ImGuiCond_Appearing, ImVec2(0.5, 0.5));
        ImGui::SetNextWindowSize(uiSize, ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(uiSize, ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));
        ToolWindow::onBeginUpdate();
    }

void SettingWindow::renderGeneralSettingsSection() {
        if (uiImHeader(__("Language"), true)) {
            uiImIndent();
                renderLanguageSettingsContent(false);
            uiImUnindent();
        }
    }

void SettingWindow::renderLanguageSettingsContent(bool embedded) {
        ensureDraftLoaded();
        if (embedded) {
            uiImLabelColored(_("Language"), vec4(0.8, 0.3, 0.3, 1));
            uiImSeperator();
            uiImIndent();
        }
        uiImLabel(_("Language"));
        uiImSameLine();
        std::string currentName = languageDraft.empty() || languageDraft == "en"
            ? _("English")
            : neLocaleCurrentName();
        auto selectedEntry = neLocaleGetEntryFor(languageDraft);
        if (selectedEntry != nullptr) currentName = selectedEntry->humanName;
        if (uiImBeginComboBox("##LANGUAGE", currentName.c_str())) {
            if (uiImSelectable(_("English").c_str(), languageDraft.empty() || languageDraft == "en")) {
                languageDraft = "en";
            }
            for (auto& entry : neLocaleGetEntries()) {
                if (uiImSelectable(entry.humanNameC(), entry.code == languageDraft)) {
                    languageDraft = entry.code;
                }
            }
            uiImEndComboBox();
        }
        uiImLabel(_("Restart may be required for all text to update."));
        if (embedded) {
            uiImUnindent();
        }
    }

void SettingWindow::renderTrackingSettingsContent(bool embedded) {
        ensureDraftLoaded();
        auto tracker = trackerDraft;
        uiImCheckbox(__("Enable tracking"), tracker->enabled);
        if (tracker->enabled) {
            tracker->update();
            bool openedEmbeddedCategory = false;
            if (embedded) {
                openedEmbeddedCategory = uiImBeginCategory("##tracker");
            }
            if (!embedded || openedEmbeddedCategory) {
                uiImLabel(_("Embedded nijitrack"));
                if(!neTracker()->launchError.empty())
                    uiImLabelColored(neTracker()->launchError,vec4(0.95,0.5,0.5,1));
                auto problem = tracker->validationError();
                if (!problem.empty()) {
                    uiImLabelColored(problem, vec4(0.95, 0.5, 0.5, 1));
                } else {
                    uiImLabel(_("Camera device"));
                    uiImSameLine();
                    auto deviceList = tracker->listDevices();
                    if (!tracker->launchError.empty()) {
                        uiImLabelColored(tracker->launchError, vec4(0.95, 0.5, 0.5, 1));
                        if (uiImButton(__("Retry camera detection"))) {
                            tracker->invalidateDeviceQuery(false);
                        }
                    }
                    auto found = std::find_if(deviceList.begin(), deviceList.end(), [&](const auto& x){return x.id == tracker->device;});
                    auto currentDeviceId = found == deviceList.end() ? -1 : std::distance(deviceList.begin(), found);
                    std::string currentDeviceName = (currentDeviceId >= 0)? deviceList[currentDeviceId].name: _("Select device...");
                    if (ImGui::BeginCombo("##device", currentDeviceName.c_str() ,ImGuiComboFlags_None)) {
                        if (!deviceList.empty()) {
                            for (auto& device : deviceList) {
                                if (uiImSelectable(device.name.c_str(), device.id == tracker->device)) {
                                    tracker->device = device.id;
                                }
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::InputInt("Camera width", &tracker->width);
                    ImGui::InputInt("Camera height", &tracker->height);
                    ImGui::InputDouble("Camera frame rate", &tracker->fps);
                    uiImCheckbox(__("Flip input"), tracker->flipped);
                    uiImCheckbox(__("Show camera tracking window"), tracker->showWindow);
                    uiImCheckbox(__("Keyboard tracking"), tracker->keyboard);
                }
                if (embedded) {
                    ImGui::Dummy(ImVec2(0, 4));
                }
                if (openedEmbeddedCategory) {
                    uiImEndCategory();
                }
            }
        }
    }

void SettingWindow::renderTrackingSettingsSection() {
        if (uiImHeader(__("Tracking"), true)) {
            uiImIndent();
                renderTrackingSettingsContent(false);
            uiImUnindent();
        }
    }

void SettingWindow::renderRenderingSettingsContent(bool embedded) {
        // Embedded View settings bypass ToolWindow::onBeginUpdate.
        neSetMeasureFPS(true);
        ensureDraftLoaded();
        if (embedded) {
            uiImLabelColored(_("V-Sync throttling"), vec4(0.8, 0.3, 0.3, 1));
            uiImSeperator();
            uiImIndent();
                uiImLabel(_("Throtting interval") + " (" + _("Experimental") + ")");
                ImGui::SliderInt("##THROTTLING", &throttlingDraft, 0, 6);
                uiImSameLine();
                uiImLabel(_(fpsText()));
            uiImUnindent();
            ImGui::Dummy(ImVec2(0, 6));

            uiImLabelColored(_("Triple buffer fallback"), vec4(0.8, 0.3, 0.3, 1));
            uiImSeperator();
            uiImIndent();
                uiImCheckbox(__("Enable triple buffer fallback"), tripleFallbackDraft);
                uiImLabel(_("Use when advanced blend is unavailable or causes issues."));
            uiImUnindent();
            return;
        }

        if (uiImHeader(__("V-Sync throttling"), true)) {
            uiImIndent();
                uiImLabel(_("Throtting interval") + " (" + _("Experimental") + ")");
                ImGui::SliderInt("##THROTTLING", &throttlingDraft, 0, 6);
                uiImSameLine();
                uiImLabel(_(fpsText()));
            uiImUnindent();
        }
        if (uiImHeader(__("Triple buffer fallback"), true)) {
            uiImIndent();
                uiImCheckbox(__("Enable triple buffer fallback"), tripleFallbackDraft);
                uiImLabel(_("Use when advanced blend is unavailable or causes issues."));
            uiImUnindent();
        }
    }

void SettingWindow::renderRenderingSettingsSection() {
        renderRenderingSettingsContent(false);
    }

void SettingWindow::onUpdate() {
        ensureDraftLoaded();
        vec2 avail = uiImAvailableSpace();
        float lhs = 196;
        float rhs = avail.x-lhs;
        const float footerHeight = 38.0f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1, 1, 1, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
        ScopeExit cleanup{[&] {
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }};

        if (uiImBeginChild("##LHS", vec2(lhs, -footerHeight), false)) {
            avail = uiImAvailableSpace();
            uiImPush(0);
            if (uiImSelectable(__("General"), selected == SelectedMode::General)) {
                selected = SelectedMode::General;
            }
            if (uiImSelectable(__("Tracking"), selected == SelectedMode::Tracking)) {
                selected = SelectedMode::Tracking;
            }
            if (uiImSelectable(__("Rendering"), selected == SelectedMode::Rendering)) {
                selected = SelectedMode::Rendering;
            }
            uiImPop();            
        }
        uiImEndChild();

        uiImSameLine(0, 0);

        if (uiImBeginChild("##RHS", vec2(rhs, -footerHeight), false)) {
            switch (selected) {
            case SelectedMode::General:
                renderGeneralSettingsSection();
                break;
            case SelectedMode::Tracking:
                renderTrackingSettingsSection();
                break;
            case SelectedMode::Rendering:
                renderRenderingSettingsSection();
                break;
            }
        }
        uiImEndChild();

        ImGui::SetCursorPosX(std::max(0.0f, uiImAvailableSpace().x - 72.0f));
        if (uiImButton(__("Apply"), vec2(64, 0))) {
            applySettings();
        }
    }

void SettingWindow::onClose() {
        trackerDraftLoaded = false;
        neSetMeasureFPS(prevMeasureFPS);
    }


SettingWindow::SettingWindow(std::function<void(int)> setter) : ToolWindow(_("Settings")), setThrottling(std::move(setter)) {
        flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoScrollbar;
        prevMeasureFPS = neGetMeasureFPS();
    }

}
