#pragma once
#include "item.hpp"
#include "nijiexpose/tracking/vspace.hpp"
namespace nijiexpose::scene {
class Scene {
public:
    std::shared_ptr<tracking::VirtualSpace> space;
    std::vector<std::shared_ptr<SceneItem>> sceneItems;
    std::string bgPath;
    std::shared_ptr<nicxlive::core::Texture> backgroundImage;
    bool shouldPostProcess=true;
    float zoneInactiveTimer=0;
    bool sleeping=false;
    void addPuppet(const std::string& path,const std::shared_ptr<Puppet>& puppet);
    void init();
    void cleanup();
    void update();
    std::shared_ptr<SceneItem> selectedSceneItem();
    bool deleteSelectedSceneItem();
    bool removeSceneItem(const std::shared_ptr<SceneItem>& item);
    void interact();
};
std::shared_ptr<Scene> insScene();
bool neGetMeasureFPS();
void neSetMeasureFPS(bool value);
double neGetFPS();
}
