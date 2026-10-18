#pragma once
#include "adaptor.hpp"
#include "udp.hpp"
#include <array>
#include <mutex>
#include <thread>
namespace nijiexpose::ft {
struct OSFData {
    double time=std::numeric_limits<double>::quiet_NaN();
    int id=0;
    std::array<float,2> cameraResolution;
    float rightEyeOpen,leftEyeOpen;
    Quaternion rightGaze,leftGaze;
    bool got3dPoints=false;
    float fit3dError;
    Vec3 translation,rawEuler;
    Quaternion rawQuaternion;
    std::array<float,68> confidence;
    std::array<std::array<float,2>,68> points;
    std::array<Vec3,70> points3d;
    std::map<std::string,float> features;
};
class OSFAdaptor final:public Adaptor {
    uint16_t port=11573;
    std::string bind="0.0.0.0";
    std::unique_ptr<UdpSocket> osf;
    std::atomic<bool> isCloseRequested=false;
    std::thread receivingThread;
    std::mutex mutex;
    OSFData data{};
    bool updated_=false,gotDataFromFetch=false;
    void receiveThread();
public:
    ~OSFAdaptor() override {stop();}
    std::string getAdaptorName() override {return "OpenSeeFace";}
    void start() override;
    void stop() override;
    void poll() override;
    bool isRunning() override {return osf!=nullptr;}
    bool isReceivingData() override {return gotDataFromFetch;}
    std::vector<std::string> getOptionNames() override {return {"osf_bind_port","osf_bind_ip"};}
};
}
