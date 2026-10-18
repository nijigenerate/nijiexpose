#pragma once
#include "ft/adaptor.hpp"
#include "nijiexpose/json.hpp"
namespace nijiexpose::tracking {
class VirtualSpaceZone {
public:
    std::string name;
    std::vector<std::shared_ptr<ft::Adaptor>> sources;
    VirtualSpaceZone()=default;
    explicit VirtualSpaceZone(std::string name):name(std::move(name)) {}
    json::Value serialize() const;
    void deserializeFromJson(const json::Value& data);
    float getBlendshapeFor(const std::string& name);
    ft::Bone getBoneFor(const std::string& name);
};
class VirtualSpace {
    std::vector<std::shared_ptr<VirtualSpaceZone>> zones;
    size_t activeZone=0;
    bool hasAnyFocus_=true;
    std::vector<std::shared_ptr<ft::Adaptor>> allSources;
    std::vector<std::string> allBlendshapes,allBones;
    void rebuildZoneList();
public:
    json::Value serialize() const;
    void deserializeFromJson(const json::Value& data);
    std::shared_ptr<VirtualSpaceZone> currentZone() { return activeZone<zones.size()?zones[activeZone]:nullptr; }
    auto getZones() const { return zones; }
    auto getAllBlendshapeNames() const { return allBlendshapes; }
    auto getAllBoneNames() const { return allBones; }
    auto& getAllSources() { return allSources; }
    bool hasAnyFocus() const { return hasAnyFocus_; }
    void addZone(std::shared_ptr<VirtualSpaceZone> zone);
    void removeZone(std::shared_ptr<VirtualSpaceZone> zone);
    void removeZoneAt(size_t idx);
    void refresh() { rebuildZoneList(); }
    bool isCurrentZoneActive();
    void update();
};
void insSaveVSpace(const std::shared_ptr<VirtualSpace>& space);
std::shared_ptr<VirtualSpace> insLoadVSpace();
}
