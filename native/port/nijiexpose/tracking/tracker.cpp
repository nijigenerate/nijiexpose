#include "tracker.hpp"
#include "nijiexpose/scene/package.hpp"
#include "ext/embedded_frame.hpp"
#include "ext/exvmc.hpp"
#include "nijiexpose/log.hpp"
#include "nijiui/core/settings.hpp"
#include "i18n/tr.hpp"
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#else
#include <SDL3/SDL.h>
#endif
#include <cmath>
#include "embedded.h"
#include <SDL3/SDL.h>
namespace nijiexpose::tracking {
namespace {std::filesystem::path path(const std::string& s){return std::filesystem::u8path(s);}std::string utf8(const std::filesystem::path& p){auto s=p.u8string();return {s.begin(),s.end()};}}
Tracker::~Tracker(){terminate();if(cameraInitialized)SDL_QuitSubSystem(SDL_INIT_CAMERA);}
std::string Tracker::launchConfigurationKey()const{return "embedded-nijitrack";}
std::string Tracker::modelPath()const{
#ifdef _WIN32
std::wstring exe(32768,L'\0');auto n=GetModuleFileNameW(nullptr,exe.data(),static_cast<DWORD>(exe.size()));exe.resize(n);return utf8(std::filesystem::path(exe).parent_path()/"models"/"face_landmarker.task");
#elif defined(__APPLE__)
const auto* base=SDL_GetBasePath(); if(!base)return {}; auto result=std::filesystem::path(base)/"../Resources/models/face_landmarker.task"; return utf8(result);
#else
const auto* base=SDL_GetBasePath(); if(!base)return {}; return utf8(std::filesystem::path(base)/"models/face_landmarker.task");
#endif
}
std::string Tracker::validationError()const{if(width<1||height<1||width>8192||height>8192||!std::isfinite(fps)||fps<=0||fps>240)return "Invalid camera resolution or frame rate.";return std::filesystem::is_regular_file(path(modelPath()))?std::string{}:"Bundled face_landmarker.task is missing.";}
void Tracker::invalidateDeviceQuery(bool reset){deviceList.clear();if(reset)device=-1;queryFailed=false;launchError.clear();deviceQueryKey.clear();}
void Tracker::ensureCamera(){if(!cameraInitialized){if(!SDL_InitSubSystem(SDL_INIT_CAMERA))throw std::runtime_error(SDL_GetError());cameraInitialized=true;}}
json::Value Tracker::serialize()const{return json::Object{{"enabled",enabled},{"flipped",flipped},{"showWindow",showWindow},{"device",device},{"width",width},{"height",height},{"fps",fps},{"keyboard",keyboard}};}
void Tracker::deserializeFromJson(const json::Value& data){auto read=[&](const char* key,auto& value){if(auto v=json::find(data,key))json::deserialize(*v,value);};read("enabled",enabled);read("flipped",flipped);read("showWindow",showWindow);read("device",device);read("width",width);read("height",height);read("fps",fps);read("keyboard",keyboard);initialized=true;}
void Tracker::update(){
    if(!embeddedSession)return;
    char error[2048]{};NtFrame frame{};int status=nt_poll(embeddedSession,&frame,error,sizeof(error));
    if(status==2){
        publishEmbeddedFrame(frame);
        if(frame.face_present && !discoveredBindings){scene::insScene()->space->refresh();discoveredBindings=true;}
    }
    if(status<=0){launchError=error;nt_stop(embeddedSession);embeddedSession=nullptr;embeddedFeed().active=false;neClearEmbeddedKeys();if(status<0)insLogErr("Tracker failed: %s",launchError);}
}
bool Tracker::handleEvent(const SDL_Event& event){return embeddedSession && nt_event(embeddedSession,&event);}
std::vector<Tracker::DeviceInfo> Tracker::listDevices(bool force){
    if(!force && (!deviceQueryKey.empty() || queryFailed))return deviceList;
    try{ensureCamera();deviceList.clear();char error[2048]{};
        auto callback=[](int id,const char* name,void* context){static_cast<Tracker*>(context)->deviceList.push_back({id,name});};
        if(!nt_devices(callback,this,error,sizeof(error)))throw std::runtime_error(error);
        deviceQueryKey=launchConfigurationKey();queryFailed=false;launchError.clear();
    }catch(const std::exception& ex){launchError=ex.what();queryFailed=true;}return deviceList;
}
void Tracker::restart(){
    terminate();launchError.clear();discoveredBindings=false;if(!enabled || device<0)return;
    launchError=validationError();if(!launchError.empty())return;
    try{ensureCamera();char error[2048]{};auto model=modelPath();NtConfig config{model.c_str(),device,flipped,showWindow};config.width=width;config.height=height;config.fps=fps;
        if(keyboard)config.key_callback=[](int down,unsigned code,void*){neSetEmbeddedKey(static_cast<int>(code),down!=0);};
        embeddedSession=nt_start(&config,error,sizeof(error));if(!embeddedSession)launchError=error;
    }catch(const std::exception& ex){launchError=ex.what();}
}
void Tracker::terminate(){if(embeddedSession){nt_stop(embeddedSession);embeddedSession=nullptr;embeddedFeed().active=false;neClearEmbeddedKeys();}}
void Tracker::setupVSpace(){
    auto space=scene::insScene()->space;
    for(auto& zone:space->getZones())for(auto& source:zone->sources)
        if(source && source->getAdaptorName()=="Embedded nijitrack"){source->start();return;}
    auto source=std::make_shared<EmbeddedAdaptor>();source->start();
    auto zone=std::make_shared<VirtualSpaceZone>("nijitrack");zone->sources.push_back(source);space->addZone(zone);
}
std::shared_ptr<Tracker>& neTracker(){static thread_local std::shared_ptr<Tracker> tracker;if(!tracker){tracker=std::make_shared<Tracker>();if(nijiui::inSettingsCanGet("tracker"))tracker->deserializeFromJson(nijiui::inSettingsGetJson("tracker"));}return tracker;}
void neInitTracker(){if(neTracker()->enabled){neTracker()->setupVSpace();neTracker()->restart();}}
void neShutdownTracker(){neTracker()->terminate();}
}
