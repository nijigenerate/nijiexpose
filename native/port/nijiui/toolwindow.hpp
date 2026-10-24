#pragma once
#include <imgui.h>
#include <memory>
#include <string>
namespace nijiui {
class ToolWindow {
    friend void inPushToolWindow(std::shared_ptr<ToolWindow>);
    friend void inPushToolWindowList(std::shared_ptr<ToolWindow>);
    friend void inPopToolWindowListAll();
    friend void inPopToolWindow();
    friend void inUpdateToolWindows();
    std::string name_;
    bool visible = true, disabled = false;
    int spawnedId = 0;
    std::string imName;
    bool hasImName = false, embeddedMode = false;
protected:
    bool onlyOne = false, drewWindow = false;
    ImGuiWindowFlags flags = 0;
    bool allowResize = true;
    float minWidth = 500, minHeight = 400;
    virtual void onUpdate() = 0;
    virtual void onBeginUpdate();
    virtual void onEndUpdate();
    virtual void onClose() {}
public:
    virtual ~ToolWindow() = default;
    explicit ToolWindow(std::string name);
    void close();
    const std::string& name() const;
    void setTitle(std::string title);
    void update();
    void updateEmbedded();
    bool isEmbedded() const;
    ImVec2 getPosition();
    void disable();
    void restore();
};
void inPushToolWindow(std::shared_ptr<ToolWindow> window);
void inPushToolWindowList(std::shared_ptr<ToolWindow> window);
void inPopToolWindowList(std::shared_ptr<ToolWindow> window);
void inPopToolWindowListAll();
void inPopToolWindow();
void inUpdateToolWindows();
std::shared_ptr<ToolWindow> inGetTopToolWindow();
}
