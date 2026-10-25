// nijiui/toolwindow.d. Copyright 2022 Inochi2D, 2024 nijigenerate. BSD-2-Clause.
#include "nijiui/toolwindow.hpp"
#include "dcompat/scope.hpp"
#include "dcompat/errors.hpp"
#include <imgui_internal.h>
#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <vector>
namespace nijiui {
namespace {
thread_local std::unique_ptr<ImGuiWindowClass> windowClass;
thread_local std::uint32_t spawnCount = 0;
thread_local std::vector<std::shared_ptr<ToolWindow>> windowStack, windowList;
}
void ToolWindow::onBeginUpdate() {
    if (!hasImName) setTitle(name());
    ImGui::SetNextWindowClass(windowClass.get());
    ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, minHeight),
        allowResize ? ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()) : ImVec2(minWidth, minHeight));
    drewWindow = ImGui::Begin(imName.c_str(), &visible, flags);
}
void ToolWindow::onEndUpdate() { ImGui::End(); }
ToolWindow::ToolWindow(std::string name) : name_(std::move(name)) { restore(); }
void ToolWindow::close() { visible = false; }
const std::string& ToolWindow::name() const { return name_; }
void ToolWindow::setTitle(std::string title) {
    name_ = std::move(title);
    imName = name_ + "###" + std::to_string(spawnedId);
    hasImName = true;
}
void ToolWindow::update() {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, disabled);
    onBeginUpdate();
    onUpdate();
    onEndUpdate();
    ImGui::PopItemFlag();
    if (disabled && !visible) visible = true;
}
void ToolWindow::updateEmbedded() {
    embeddedMode = true;
    nijiexpose::dcompat::ScopeExit reset([&] { embeddedMode = false; });
    onUpdate();
}
bool ToolWindow::isEmbedded() const { return embeddedMode; }
ImVec2 ToolWindow::getPosition() { return ImGui::GetWindowPos(); }
void ToolWindow::disable() {
    flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
    disabled = true;
}
void ToolWindow::restore() {
    disabled = false;
    flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    windowClass = std::make_unique<ImGuiWindowClass>();
    windowClass->ViewportFlagsOverrideClear = ImGuiViewportFlags_NoDecoration | ImGuiViewportFlags_NoTaskBarIcon;
    windowClass->ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
}
void inPushToolWindow(std::shared_ptr<ToolWindow> window) {
    window->spawnedId = std::bit_cast<std::int32_t>(spawnCount++);
    if (window->onlyOne) for (auto& win : windowStack) if (win->name() == window->name()) return;
    if (!windowStack.empty()) windowStack.back()->disable();
    windowStack.push_back(std::move(window));
}
void inPushToolWindowList(std::shared_ptr<ToolWindow> window) {
    window->spawnedId = std::bit_cast<std::int32_t>(spawnCount++);
    if (window->onlyOne) for (auto& win : windowList) if (win->name() == window->name()) return;
    windowList.push_back(std::move(window));
}
void inPopToolWindowList(std::shared_ptr<ToolWindow> window) {
    auto i = std::find(windowList.begin(), windowList.end(), window);
    if (i != windowList.end()) {
        if (windowList.size() == 1) windowList.clear();
        else windowList.erase(i);
    }
}
void inPopToolWindowListAll() {
    const auto windows = windowList;
    for (auto& window : windows) { window->onClose(); window->visible = false; }
    windowList.clear();
}
void inPopToolWindow() {
    if (windowStack.empty()) throw nijiexpose::dcompat::RangeError{};
    windowStack.back()->onClose();
    windowStack.pop_back();
    if (!windowStack.empty()) windowStack.back()->restore();
}
void inUpdateToolWindows() {
    int id = 0;
    (void)id;
    const auto stack = windowStack;
    for (auto& window : stack) { window->update(); if (!window->visible) inPopToolWindow(); }
    std::vector<std::shared_ptr<ToolWindow>> closedWindows;
    const auto windows = windowList;
    for (auto& window : windows) { window->update(); if (!window->visible) closedWindows.push_back(window); }
    for (auto& window : closedWindows) inPopToolWindowList(window);
}
std::shared_ptr<ToolWindow> inGetTopToolWindow() { return windowStack.empty() ? nullptr : windowStack.back(); }
}
