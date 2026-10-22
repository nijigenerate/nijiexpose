#pragma once
#include "json.hpp"
#include <memory>
#include <SDL3/SDL_events.h>
namespace nijiexpose::tracking {
class Tracker {
    std::string deviceQueryKey;
    bool queryFailed=false,cameraInitialized=false,discoveredBindings=false;
    void* embeddedSession=nullptr;
    void ensureCamera();
public:
    bool enabled=false,flipped=true,showWindow=false,initialized=false;
    bool keyboard=true;
    int device=-1,width=640,height=480;
    double fps=30;
    std::string launchError;
    struct DeviceInfo {int id=0;std::string name;};
    std::vector<DeviceInfo> deviceList;
    Tracker()=default;
    ~Tracker();
    Tracker(const Tracker&)=delete;
    Tracker& operator=(const Tracker&)=delete;
    std::string modelPath() const;
    std::string launchConfigurationKey() const;
    std::string validationError() const;
    void invalidateDeviceQuery(bool resetSelection=true);
    json::Value serialize() const;
    void deserializeFromJson(const json::Value&);
    void update();
    bool handleEvent(const SDL_Event&);
    bool running() const{return embeddedSession!=nullptr;}
    std::vector<DeviceInfo> listDevices(bool force=false);
    void restart();
    void terminate();
    void setupVSpace();
};
std::shared_ptr<Tracker>& neTracker();
void neInitTracker();
void neShutdownTracker();
}
