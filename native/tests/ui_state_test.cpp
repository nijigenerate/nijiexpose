#include "nijiui/panel.hpp"
#include "nijiui/toolwindow.hpp"
#include "nijiui/core/app.hpp"
#include "nijiui/core/settings.hpp"
#include "dcompat/errors.hpp"
#include <imgui_internal.h>
#include <cstdio>
#include <stdexcept>
#define CHECK(value) do { if (!(value)) throw std::runtime_error(#value); } while (0)
namespace {
struct TestPanel : nijiui::Panel {
    int initialized = 0, updated = 0, embedded = 0;
    bool throwNext = false;
    TestPanel() : Panel("test panel", "Test panel", false) {}
    void onInit() override { ++initialized; }
    void onUpdate() override {
        ++updated;
        if (isEmbedded()) ++embedded;
        if (throwNext) { throwNext = false; throw std::runtime_error("embedded probe"); }
        ImGui::TextUnformatted("panel");
    }
};
struct TestTool : nijiui::ToolWindow {
    int updated = 0, closed = 0, embedded = 0;
    bool closeNext = false, throwNext = false;
    explicit TestTool(const char* name, bool single = false) : ToolWindow(name) { onlyOne = single; }
    void onUpdate() override {
        ++updated;
        if (isEmbedded()) ++embedded;
        if (throwNext) { throwNext = false; throw std::runtime_error("embedded probe"); }
        if (closeNext) { closeNext = false; close(); }
        ImGui::TextUnformatted("tool");
    }
    void onClose() override { ++closed; }
};
}
int main() {
    try {
#ifndef IMGUI_HAS_DOCK
#error The D UI requires the official docking branch.
#endif
        using namespace nijiui;
        inSetApplication({"net.nijilive.nijiexpose", "nijiexpose", "nijiexpose"});
        inSettingsLoad();
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.DisplaySize = ImVec2(1280,720);
        io.DeltaTime = 1.0f/60;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        auto panel = std::make_shared<TestPanel>();
        inAddPanel(panel);
        inInitPanels();
        CHECK(panel->initialized == 1 && !panel->visible && !panel->getDefaultVisibility());
        auto first = std::make_shared<TestTool>("first");
        auto second = std::make_shared<TestTool>("second", true);
        auto duplicate = std::make_shared<TestTool>("second", true);
        auto listed = std::make_shared<TestTool>("listed");
        auto listed2 = std::make_shared<TestTool>("listed2");
        inPushToolWindow(first);
        inPushToolWindow(second);
        inPushToolWindow(duplicate);
        CHECK(inGetTopToolWindow() == second);
        inPushToolWindowList(listed);
        inPushToolWindowList(listed2);
        listed->closeNext = true;
        first->closeNext = true; // A disabled underlying window must remain open.
        for (int frame = 0; frame < 6; ++frame) {
            ImGui::NewFrame();
            ImGui::Begin("embedded host");
            if (frame == 1) { panel->visible = true; second->closeNext = true; }
            if (frame == 2) { inSetPanelsSuspended(true); first->closeNext = true; }
            if (frame == 3) { inSetPanelsSuspended(false); panel->visible = false; panel->alwaysVisible = true; inPopToolWindowListAll(); }
            if (frame == 4) {
                panel->throwNext = true;
                try { panel->updateEmbedded(); } catch (const std::runtime_error&) {}
                CHECK(!panel->isEmbedded() && panel->embedded == 1);
                first->throwNext = true;
                try { first->updateEmbedded(); } catch (const std::runtime_error&) {}
                CHECK(!first->isEmbedded() && first->embedded == 1);
            } else {
                if (frame == 5) { panel->visible = true; panel->alwaysVisible = false; }
                inUpdatePanels();
            }
            inUpdateToolWindows();
            if (frame == 0) CHECK(inGetTopToolWindow() == second && panel->updated == 0);
            if (frame == 1) CHECK(inGetTopToolWindow() == first && panel->updated == 1);
            if (frame == 2) CHECK(!inGetTopToolWindow() && panel->updated == 1);
            ImGui::End();
            ImGui::Render();
        }
        CHECK(panel->updated == 4 && first->updated == 4 && second->updated == 2);
        CHECK(first->closed == 1 && second->closed == 1 && duplicate->updated == 0);
        CHECK(listed->updated == 1 && listed->closed == 0);
        CHECK(listed2->updated == 3 && listed2->closed == 1);
        CHECK(inSettingsGet<bool>("test panel.visible"));
        bool caughtBounds = false;
        try { inPopToolWindow(); } catch (const nijiexpose::dcompat::RangeError&) { caughtBounds = true; }
        CHECK(caughtBounds);
        inPanels.clear();
        for (auto* texture : ImGui::GetPlatformIO().Textures) texture->SetStatus(ImTextureStatus_Destroyed);
        ImGui::DestroyContext();
        std::puts("Panel visibility, suspension, embedded cleanup, tool stack/list and close callbacks passed");
    } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
