#pragma once
#include "adaptor.hpp"
#include "udp.hpp"
#include <atomic>
#include <mutex>
#include <thread>
namespace nijiexpose::ft {
struct PhoneTrackingData {
    std::map<std::string,float> blendshapes;
    Vec3 headPos{},headRot{},leftEye{},rightEye{};
    std::string indata;
    PhoneTrackingData()=default;
    PhoneTrackingData(std::string datastr,char delimiter);
};
// Both original receiver implementations have the same control flow; the
// protocol delimiter, request packet, send port and shape names differ.
class PhoneAdaptor: public Adaptor {
    bool fm3d;
    std::string phoneIP;
    std::unique_ptr<UdpSocket> sender,dataIn;
    std::atomic<bool> isCloseRequested=false;
    std::thread listeningThread;
    std::mutex mutex;
    PhoneTrackingData data;
    bool updated_=false,gotDataFromFetch=false;
    size_t dataPacketsReceivedTotal=0;
    void listenThread();
    void request();
protected:
    explicit PhoneAdaptor(bool fm3d):fm3d(fm3d) {}
public:
    ~PhoneAdaptor() override { stop(); }
    std::string getAdaptorName() override { return fm3d?"Facemotion3D":"iFacialMocap"; }
    void start() override;
    void stop() override;
    void poll() override;
    bool isRunning() override { return dataIn!=nullptr; }
    bool isReceivingData() override { return gotDataFromFetch; }
    std::vector<std::string> getOptionNames() override { return {"phoneIP"}; }
};
class IFMAdaptor final:public PhoneAdaptor { public: IFMAdaptor():PhoneAdaptor(false) {} };
class FM3DAdaptor final:public PhoneAdaptor { public: FM3DAdaptor():PhoneAdaptor(true) {} };
}
