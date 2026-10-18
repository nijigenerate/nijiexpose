// Independent facetrack-d 0.8.0 ft.adaptors.vtsproto UDP tracking adaptor.
#include "vtsproto.hpp"
#include "quaternion.hpp"
#include "nijiexpose/json.hpp"
#include <chrono>
#include <array>
#include <charconv>
namespace nijiexpose::ft {
void VTSAdaptor::listenThread() {
    std::array<uint8_t,65535> buff{};
    while(!isCloseRequested) {
        try {
            int recvBytes=vtsIn->receive(buff);
            if(recvBytes!=-1 && recvBytes<=static_cast<int>(buff.size())) {
                auto object=json::parse({reinterpret_cast<char*>(buff.data()),static_cast<size_t>(recvBytes)});
                VTSRawTrackingData next;
                auto field=[&](std::string_view name,auto& output) {if(auto v=json::find(object,name)) json::deserialize(*v,output);};
                field("Timestamp",next.timestamp); field("Hotkey",next.hotkey);field("FaceFound",next.faceFound);
                auto vector=[&](std::string_view name,Vec3& output) {
                    if(auto v=json::find(object,name)) {
                        if(auto x=json::find(*v,"x")) json::deserialize(*x,output.x);
                        if(auto y=json::find(*v,"y")) json::deserialize(*y,output.y);
                        if(auto z=json::find(*v,"z")) json::deserialize(*z,output.z);
                    }
                };
                vector("Rotation",next.rotation);vector("Position",next.position);
                if(auto shapes=json::find(object,"BlendShapes");shapes && !shapes->is_null()) for(auto& shape:shapes->as_array()) {
                    std::string key; float value=std::numeric_limits<float>::quiet_NaN();
                    if(auto k=json::find(shape,"k")) json::deserialize(*k,key);
                    if(auto v=json::find(shape,"v")) json::deserialize(*v,value);
                    next.blendShapesDict[key]=value;
                }
                std::lock_guard lock(mutex);updated_=true;data=std::move(next);
            }
        } catch(const std::exception&) {std::this_thread::sleep_for(std::chrono::milliseconds(100));}
    }
}
void VTSAdaptor::sendThread() {
    float clampedFactor=std::clamp(pollingFactor,1.f,5.f);
    int sleepMs=std::clamp(static_cast<int>(1000.0/(5.f*clampedFactor)),10,5000);
    std::string request=json::serialize(json::Object{{"messageType","iOSTrackingDataRequest"},{"time",1.f/clampedFactor},{"sentBy",appName},{"ports",json::Array{21412}}});
    while(!isCloseRequested) {
        try {vtsOut->sendTo({reinterpret_cast<const uint8_t*>(request.data()),request.size()},phoneIP,21412);} catch(const std::runtime_error&) {}
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    }
}
void VTSAdaptor::start() {
    if(options.contains("appName")) {
        appName=options.at("appName");
        if(appName.empty()) throw std::runtime_error("App Name can't be empty.");
        if(appName.size()>32) throw std::runtime_error("App Name can't be longer than 32 characters.");
    }
    if(options.contains("phoneIP")) phoneIP=options.at("phoneIP"); else return;
    if(options.contains("pollingFactor")) {
        const auto& text=options.at("pollingFactor");
        auto result=std::from_chars(text.data(),text.data()+text.size(),pollingFactor);
        if(result.ec!=std::errc{} || result.ptr!=text.data()+text.size()) return;
    }
    if(isRunning()) stop();
    isCloseRequested=false;
    {std::lock_guard lock(mutex);updated_=false;data={};}
    vtsOut=std::make_unique<UdpSocket>();vtsOut->sendTimeout(16);
    vtsIn=std::make_unique<UdpSocket>();vtsIn->timeout(16);vtsIn->bind("0.0.0.0",21412);
    dataPacketsReceivedTotal=0;dataPacketsReceivedInLastSecond=0;
    sendingThread=std::thread([this]{sendThread();});listeningThread=std::thread([this]{listenThread();});
}
void VTSAdaptor::stop() {
    if(isRunning()) {
        isCloseRequested=true;
        if(sendingThread.joinable())sendingThread.join();if(listeningThread.joinable())listeningThread.join();
        if(vtsIn){vtsIn->close();vtsIn.reset();}vtsOut->close();vtsOut.reset();
    }
}
void VTSAdaptor::poll() {
    if(!isRunning()) return;
    bool updated;{std::lock_guard lock(mutex);updated=updated_;}
    if(updated) {
        VTSRawTrackingData next;{std::lock_guard lock(mutex);updated_=false;next=data;}
        dataLossCounter=0;gotDataFromFetch=next.faceFound;
        auto radians=[](float x){return (std::numbers::pi/180)*x;};
        bones["Head"]={{next.position.x*-1,next.position.y,next.position.z},eulerRotation(radians(next.rotation.z),radians(next.rotation.y),radians(next.rotation.x))};
        blendshapes=next.blendShapesDict;
        // Source-generated expressions retain the three original format branches.
        auto clamp=[](auto v,auto lo,auto hi){using T=std::common_type_t<decltype(v),decltype(lo),decltype(hi)>;return std::clamp(T(v),T(lo),T(hi));};
        #include "vts_blendshapes.inc"
    } else {++dataLossCounter;if(dataLossCounter>RECV_TIMEOUT)gotDataFromFetch=false;}
}
}
