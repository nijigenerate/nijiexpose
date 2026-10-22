// Direct translation of dlang_archive/source/nijiexpose/windows/spaceedit.d (win32-full).
#include "spaceedit.hpp"
#include "scene/package.hpp"
#include "tracking/tracker.hpp"
#include "panels/tracking_state.hpp"
#include "nijiui/widgets/input.hpp"
#include "nijiui/widgets/category.hpp"
#include "nijiui/widgets/dialog.hpp"
#include "ft/vmc.hpp"
#include "ft/vtsproto.hpp"
#include "ft/phizosc.hpp"
#include "ft/openseeface.hpp"
#include "ft/phone.hpp"
#include "dcompat/string.hpp"
#include "i18n/tr.hpp"
#include <functional>
namespace nijiexpose::windows {
using namespace nijiui; using namespace tracking; using namespace scene; using namespace panels; using namespace ft;
using i18n::_; using i18n::__;
namespace {struct ScopeExit {std::function<void()> fn; ~ScopeExit(){fn();}};}
void SpaceEditor::addZone() {
        if (newZoneName.empty()) {
            uiImDialog(__("Error"), "Can't create a zone without a name!");
            return;
        }
        newZoneName = std::string(dcompat::fromStringz(newZoneName));

        std::shared_ptr<VirtualSpaceZone> zone = std::make_shared<VirtualSpaceZone>(newZoneName);
        insScene()->space->addZone(zone);

        newZoneName.clear();
    }

void SpaceEditor::switchZone(std::shared_ptr<VirtualSpaceZone> zone) {
        editingZone = zone;
        refreshOptionsList();
    }

void SpaceEditor::ensureEditingZone() {
        if (editingZone != nullptr) return;
        auto zones = insScene()->space->getZones();
        if (zones.size() > 0) {
            switchZone(zones[0]);
        }
    }

void SpaceEditor::refreshOptionsList() {
        options.clear();
        if (!editingZone) return;

        for (size_t i=0, sourceCount=editingZone->sources.size(); i<sourceCount; ++i) {
            if (!editingZone->sources[i]) continue;
            options[editingZone->sources[i]] = editingZone->sources[i]->getOptions();
            options[editingZone->sources[i]]["appName"] = "nijiexpose";
        }
    }

void SpaceEditor::adaptorDelete(size_t idx) {
        if (editingZone->sources[idx]) editingZone->sources[idx]->stop();
        editingZone->sources.erase(editingZone->sources.begin()+idx);
    }

void SpaceEditor::adaptorMenu(size_t idx) {
        uiImRightClickPopup("AdaptorPopup");

        if (uiImBeginPopup("AdaptorPopup")) {
            if (uiImMenuItem(__("Delete")))
                adaptorDelete(idx);
            uiImEndPopup();
        }
    }

void SpaceEditor::zoneMenu(size_t idx) {
        uiImRightClickPopup("AdaptorPopup");

        if (uiImBeginPopup("AdaptorPopup")) {
            if (uiImMenuItem(__("Delete"))) {
                for (auto& source : insScene()->space->getZones()[idx]->sources) {
                    if (source) source->stop();
                }
                insScene()->space->removeZoneAt(idx);
            }
            uiImEndPopup();
        }
    }

void SpaceEditor::adaptorHint(std::shared_ptr<Adaptor>& source) {
        std::string portHint = "";
        if (auto vts = std::dynamic_pointer_cast<IFMAdaptor>(source)) {
            portHint = _("iFacialMocap Adpator would listen on udp port 49983");
        }
        if (!portHint.empty()) {
            uiImLabel(portHint + "\n" + _("Make sure the port is not blocked by firewall."));
        }
    }

void SpaceEditor::adaptorSelect(size_t i, std::shared_ptr<Adaptor>& source, const char* adaptorName) {
        if (uiImBeginComboBox("ADAPTOR_COMBO", adaptorName)) {
            if (uiImSelectable("VTubeStudio")) {
                if (source) source->stop();
                source = std::make_shared<VTSAdaptor>();
                editingZone->sources[i] = source;
                refreshOptionsList();
            }
            if (uiImSelectable("VMC")) {
                if (source) source->stop();
                source = std::make_shared<VMCAdaptor>();
                editingZone->sources[i] = source;
                refreshOptionsList();
            }
            if (uiImSelectable("Phiz OSC")) {
                if (source) source->stop();
                source = std::make_shared<PhizOSCAdaptor>();
                editingZone->sources[i] = source;
                refreshOptionsList();
            }
            if (uiImSelectable("OpenSeeFace")) {
                if (source) source->stop();
                source = std::make_shared<OSFAdaptor>();
                editingZone->sources[i] = source;
                refreshOptionsList();
            }
            if (uiImSelectable("iFacialMocap")) {
                if (source) source->stop();
                source = std::make_shared<IFMAdaptor>();
                editingZone->sources[i] = source;
                refreshOptionsList();
            }
            if (uiImSelectable("Facemotion3D")) {
                if (source) source->stop();
                source = std::make_shared<FM3DAdaptor>();
                editingZone->sources[i] = source;
                refreshOptionsList();
            }



            uiImEndComboBox();
        }
    }


void SpaceEditor::applyChanges() {
        insScene()->space->refresh();
        insSaveVSpace(insScene()->space);
        neTrackingPanelReset();
        insTrackingPanelRefresh();
        if (neTracker()->enabled) {
            neTracker()->setupVSpace();
        }
    }

void SpaceEditor::renderEditorSection(bool showApplyButton) {
        vec2 avail = uiImAvailableSpace();
        float lhs = 196;
        float rhs = avail.x - lhs;
        const float footerHeight = showApplyButton ? 38.0f : 0.0f;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1, 1, 1, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
        ScopeExit cleanup{[&] {
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }};

        if (uiImBeginChild("##LHS", vec2(lhs, -footerHeight), false)) {
            avail = uiImAvailableSpace();
            auto zoneList = insScene()->space->getZones();
            for (size_t i=0; i<zoneList.size(); ++i) {
                auto& zone=zoneList[i];
                uiImPush(static_cast<int>(i));
                    if (uiImSelectable(zone->name.c_str(), zone == editingZone)) {
                        switchZone(zone);
                    }
                    zoneMenu(i);
                uiImPop();
            }

            std::string editingZoneName = newZoneName;
            if (uiImInputText("###ZONE_NAME", avail.x - 24, editingZoneName)) {
                try {
                    newZoneName = std::string(dcompat::fromStringz(editingZoneName));
                } catch (const std::exception&) {}
            }
            uiImSameLine(0, 0);
            if (uiImButton(__("\ue145"), vec2(24, 24))) addZone();
        }
        uiImEndChild();

        uiImSameLine(0, 0);

        if (uiImBeginChild("##RHS", vec2(rhs, -footerHeight), false)) {
            if (!editingZone) {
                uiImLabel(_("No zone selected for editing..."));
            } else {
                uiImPush(static_cast<int>(reinterpret_cast<uintptr_t>(editingZone.get())));
                    std::string editingZoneName = editingZone->name;
                    if (uiImInputText("###ZoneName", avail.x / 2, editingZoneName)) {
                        try {
                            editingZone->name = std::string(dcompat::fromStringz(editingZoneName));
                        } catch (const std::exception&) {}
                    }

                    uiImSeperator();
                    uiImNewLine();

                    uiImIndent();
                        for (size_t i=0, sourceCount=editingZone->sources.size(); i<sourceCount; ++i) {
                            if (i >= editingZone->sources.size()) continue;

                            uiImPush(static_cast<int>(i));
                                auto source = editingZone->sources[i];
                                std::string adaptorNameStorage = !source ? __("Unset") : source->getAdaptorName();
                const char* adaptorName = adaptorNameStorage.c_str();

                                if (!source) {
                                    if (uiImHeader(adaptorName, true)) {
                                        adaptorMenu(i);
                                        uiImIndent();
                                            adaptorSelect(i, source, adaptorName);
                                        uiImUnindent();
                                    } else {
                                        adaptorMenu(i);
                                    }
                                } else {
                                    if (uiImHeader(adaptorName, true)) {
                                        adaptorMenu(i);
                                        uiImIndent();
                                            avail = uiImAvailableSpace();
                                            adaptorSelect(i, source, adaptorName);
                                            ImGui::NewLine();

                                            for (auto& option : source->getOptionNames()) {
                                                if (option == "appName") continue;
                                                if (option == "address") continue;

                                                if (!options[source].contains(option)) options[source][option] = "";
                                                uiImLabel(option);
                                                std::string optionString = options[source][option];
                                                if (uiImInputText(option, avail.x / 2, optionString)) {
                                                    options[source][option] = std::string(dcompat::fromStringz(optionString));
                                                }
                                            }

                                            adaptorHint(source);

                                            if (uiImButton(__("Save Changes"))) {
                                                try {
                                                    source->setOptions(options[source]);
                                                    source->stop();
                                                    source->start();
                                                } catch (const std::exception& ex) {
                                                    uiImDialog(__("Error"), ex.what());
                                                }
                                            }

                                            uiImSameLine(0, 40);
                                            if (uiImButton(__("Delete"))) {
                                                adaptorDelete(i);
                                            }
                                        uiImUnindent();
                                    } else {
                                        adaptorMenu(i);
                                    }
                                }
                            uiImPop();
                        }
                    uiImUnindent();

                    avail = uiImAvailableSpace();
                    if (uiImButton(__("\ue145"), vec2(avail.x, 24))) {
                        editingZone->sources.resize(editingZone->sources.size()+1);
                        refreshOptionsList();
                    }
                uiImPop();
            }
        }
        uiImEndChild();

        if (showApplyButton) {
            ImGui::SetCursorPosX(std::max(0.0f, uiImAvailableSpace().x - 72.0f));
            if (uiImButton(__("Apply"), vec2(64, 0))) {
                applyChanges();
            }
        }
    }

void SpaceEditor::renderEmbeddedSection() {
        ensureEditingZone();

        auto zones = insScene()->space->getZones();
        if (uiImBeginCategory(_("Zone").c_str(), UiImCategoryFlags::NoCollapse)) {
            int zoneToDelete = -1;
            for (size_t idx=0; idx<zones.size(); ++idx) {
                auto zone=zones[idx];
                uiImPush(static_cast<int>(idx));
                    vec2 avail = uiImAvailableSpace();
                    float rowWidth = std::max(0.0f, avail.x - 38.0f);
                    bool selected = zone == editingZone;
                    if (ImGui::Selectable(zone->name.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(rowWidth, 0))) {
                        switchZone(zone);
                    }
                    uiImSameLine();
                    if (uiImButton(__("\ue872"), vec2(28, 0))) {
                        zoneToDelete = static_cast<int>(idx);
                    }
                uiImPop();
            }

            if (zoneToDelete >= 0) {
                auto zonesNow = insScene()->space->getZones();
                bool deletedSelected = editingZone == zonesNow[zoneToDelete];
                for (auto& source : zonesNow[zoneToDelete]->sources) {
                    if (source) source->stop();
                }
                insScene()->space->removeZoneAt(static_cast<size_t>(zoneToDelete));
                if (deletedSelected) {
                    editingZone.reset();
                }
                ensureEditingZone();
                refreshOptionsList();
                uiImEndCategory();
                return;
            }

            std::string pendingZoneName = newZoneName;
            vec2 avail = uiImAvailableSpace();
            if (uiImInputText("##EMBEDDED_ZONE_NAME", std::max(0.0f, avail.x - 38.0f), pendingZoneName)) {
                try {
                    newZoneName = std::string(dcompat::fromStringz(pendingZoneName));
                } catch (const std::exception&) {}
            }
            uiImSameLine();
            if (uiImButton(__("\ue145"), vec2(28, 0))) {
                addZone();
                ensureEditingZone();
            }

            if (!editingZone) {
                uiImLabel(_("No zone selected for editing..."));
                uiImEndCategory();
                return;
            }
            uiImEndCategory();
        }

        for (size_t i=0, sourceCount=editingZone->sources.size(); i<sourceCount; ++i) {
            if (i >= editingZone->sources.size()) continue;

            uiImPush(static_cast<int>(i));
                auto source = editingZone->sources[i];
                std::string adaptorNameStorage = !source ? __("Unset") : source->getAdaptorName();
                const char* adaptorName = adaptorNameStorage.c_str();

                if (!source) {
                    if (uiImBeginCategory(adaptorName)) {
                        adaptorMenu(i);
                        ImVec2 headerStart;
                        headerStart = ImGui::GetItemRectMin();
                        ImVec2 headerEnd;
                        headerEnd = ImGui::GetItemRectMax();
                        float deleteX = headerEnd.x - 32.0f;
                        float iconY = headerStart.y + 1.0f;
                        ImGui::SetCursorScreenPos(ImVec2(deleteX, iconY));
                        if (uiImButton(__("\ue872"), vec2(28, 0))) {
                            adaptorDelete(i);
                            uiImEndCategory();
                            uiImPop();
                            continue;
                        }
                        adaptorSelect(i, source, adaptorName);
                    }
                    uiImEndCategory();
                } else {
                    if (uiImBeginCategory(adaptorName)) {
                        adaptorMenu(i);
                        ImVec2 headerStart;
                        headerStart = ImGui::GetItemRectMin();
                        ImVec2 headerEnd;
                        headerEnd = ImGui::GetItemRectMax();
                        float iconY = headerStart.y + 1.0f;
                        float deleteX = headerEnd.x - 32.0f;
                        float saveX = deleteX - 32.0f;

                        ImGui::SetCursorScreenPos(ImVec2(saveX, iconY));
                        if (uiImButton(__("\ue161"), vec2(28, 0))) {
                            try {
                                source->setOptions(options[source]);
                                source->stop();
                                source->start();
                            } catch (const std::exception& ex) {
                                uiImDialog(__("Error"), ex.what());
                            }
                        }
                        ImGui::SetCursorScreenPos(ImVec2(deleteX, iconY));
                        if (uiImButton(__("\ue872"), vec2(28, 0))) {
                            adaptorDelete(i);
                            uiImEndCategory();
                            uiImPop();
                            continue;
                        }

                        vec2 avail = uiImAvailableSpace();
                        adaptorSelect(i, source, adaptorName);
                        ImGui::NewLine();

                        for (auto& option : source->getOptionNames()) {
                            if (option == "appName") continue;
                            if (option == "address") continue;

                            if (!options[source].contains(option)) options[source][option] = "";
                            uiImLabel(option);
                            std::string optionString = options[source][option];
                            if (uiImInputText(option, avail.x / 2, optionString)) {
                                options[source][option] = std::string(dcompat::fromStringz(optionString));
                            }
                        }

                        adaptorHint(source);
                    }
                    uiImEndCategory();
                }
            uiImPop();
        }

        vec2 avail = uiImAvailableSpace();
        if (uiImButton(__("Add Source"), vec2(std::min(avail.x, 220.0f), 28))) {
            editingZone->sources.resize(editingZone->sources.size()+1);
            refreshOptionsList();
        }
    }

void SpaceEditor::onBeginUpdate() {
        ImVec2 wpos = ImVec2(
            ImGui::GetMainViewport()->Pos.x + (ImGui::GetMainViewport()->Size.x / 2),
            ImGui::GetMainViewport()->Pos.y + (ImGui::GetMainViewport()->Size.y / 2)
        );

        ImVec2 uiSize = ImVec2(800, 600);

        ImGui::SetNextWindowPos(wpos, ImGuiCond_Appearing, ImVec2(0.5, 0.5));
        ImGui::SetNextWindowSize(uiSize, ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(uiSize, ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));
        ToolWindow::onBeginUpdate();
    }

void SpaceEditor::onUpdate() {
        renderEditorSection(true);
    }

SpaceEditor::SpaceEditor() : ToolWindow(_("Virtual Space")) {
        flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoScrollbar;
    }
}

