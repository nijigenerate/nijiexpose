#pragma once
#include "adaptor.hpp"
#include "udp.hpp"
#include <mutex>
#include <thread>
namespace nijiexpose::ft {
struct VTSRawTrackingData {
    int64_t timestamp=0;
    int hotkey=-1;
    bool faceFound=false;
    Vec3 rotation,position;
    std::map<std::string,float> blendShapesDict;
    VTSRawTrackingData() {
        float nan=std::numeric_limits<float>::quiet_NaN(); rotation=position={nan,nan,nan};
    }
};
class VTSAdaptor final:public Adaptor {
    size_t dataPacketsReceivedTotal=0,dataPacketsReceivedInLastSecond=0;
    std::string appName="facetrack-d",phoneIP;
    float pollingFactor=1;
    std::unique_ptr<UdpSocket> vtsIn,vtsOut;
    std::atomic<bool> isCloseRequested=false;
    std::thread sendingThread,listeningThread;
    std::mutex mutex;
    VTSRawTrackingData data;
    bool updated_=false,gotDataFromFetch=false;
    void listenThread();
    void sendThread();
public:
    ~VTSAdaptor() override {stop();}
    std::string getAdaptorName() override {return "VTubeStudio";}
    void start() override;
    void stop() override;
    void poll() override;
    bool isRunning() override {return vtsOut!=nullptr;}
    bool isReceivingData() override {return gotDataFromFetch;}
    std::vector<std::string> getOptionNames() override {return {"phoneIP","appName","pollingFactor"};}
};
}
