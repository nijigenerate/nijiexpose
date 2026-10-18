// Direct port of source/nijiexpose/tracking/vspace.d (BSD-2-Clause).
#include "vspace.hpp"
#include "ft/quaternion.hpp"
#include "nijiexpose/log.hpp"
#include <algorithm>
namespace nijiexpose::tracking {
void VirtualSpace::rebuildZoneList() {
    allSources.clear(); allBlendshapes.clear(); allBones.clear();
    for(auto& zone:zones) {
        insLogInfo("Found zone %s",zone->name);
        allSources.insert(allSources.end(),zone->sources.begin(),zone->sources.end());
    }
    for(auto& source:allSources) {
        if(!source) continue;
        source->poll();
        for(auto& [key,value]:source->getBlendshapes()) if(std::find(allBlendshapes.begin(),allBlendshapes.end(),key)==allBlendshapes.end()) allBlendshapes.push_back(key);
        for(auto& [key,value]:source->getBones()) if(std::find(allBones.begin(),allBones.end(),key)==allBones.end()) allBones.push_back(key);
    }
}
void VirtualSpace::addZone(std::shared_ptr<VirtualSpaceZone> zone) { zones.push_back(std::move(zone)); rebuildZoneList(); }
void VirtualSpace::removeZone(std::shared_ptr<VirtualSpaceZone> zone) {
    auto it=std::find(zones.begin(),zones.end(),zone); if(it!=zones.end()) zones.erase(it);
    rebuildZoneList();
}
void VirtualSpace::removeZoneAt(size_t idx) { if(idx<zones.size()) zones.erase(zones.begin()+idx); rebuildZoneList(); }
bool VirtualSpace::isCurrentZoneActive() {
    auto current=currentZone();
    return current && !current->sources.empty() && current->sources[0] && current->sources[0]->isReceivingData();
}
void VirtualSpace::update() {
    if(zones.empty()) { hasAnyFocus_=false; return; }
    if(!currentZone()) activeZone=0;
    for(auto& zone:zones) for(auto& source:zone->sources) if(source) source->poll();
    hasAnyFocus_=isCurrentZoneActive();
    if(!hasAnyFocus_) for(size_t i=0;i<zones.size();++i) {
        auto& zone=zones[i]; if(zone->sources.empty()) continue;
        if(zone->sources[0] && zone->sources[0]->isReceivingData()) activeZone=i;
    }
}
float VirtualSpaceZone::getBlendshapeFor(const std::string& name) {
    if(sources.empty()) return 0;
    float sum=0,count=0;
    for(auto& source:sources) {
        if(!source) continue;
        auto& blend=source->getBlendshapes();
        if(blend.contains(name)) { sum+=blend.at(name); count+=1; }
    }
    if(sum==0 || count==0) return 0;
    return sum/count;
}
ft::Bone VirtualSpaceZone::getBoneFor(const std::string& name) {
    ft::Bone sum{};
    if(sources.empty() || !sources[0]) return sum;
    if(!sources[0]->getBones().contains(name)) return sum;
    sum=sources[0]->getBones().at(name);
    if(sources.size()>1) {
        float count=1;
        for(size_t i=1;i<sources.size();++i) {
            auto& source=sources[i]; if(!source) continue;
            if(source->getBones().contains(name)) {
                count+=1;
                const auto b=source->getBones().at(name);
                sum.position.x+=b.position.x; sum.position.y+=b.position.y; sum.position.z+=b.position.z;
                sum.rotation=ft::slerp(sum.rotation,b.rotation,1.f/(count+1));
            }
        }
        sum.position.x/=count; sum.position.y/=count; sum.position.z/=count;
    }
    return sum;
}
}
