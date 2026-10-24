#pragma once
#include "adaptor.hpp"
#include "udp.hpp"
#include <array>
#include <mutex>
#include <thread>
#include <atomic>
namespace nijiexpose::ft {
class LLFAdaptor final:public Adaptor {
    std::unique_ptr<UdpSocket> llfIn;
    std::atomic<bool> isCloseRequested=false;
    std::thread listeningThread;
    std::mutex mutex;
    std::array<float,61> data{};
    bool updated_=false;
    bool gotDataFromFetch=false;
    void listenThread();
public:
    ~LLFAdaptor() override { stop(); }
    std::string getAdaptorName() override { return "LiveLinkFace/MeFaMo Receiver"; }
    void start() override;
    void stop() override;
    void poll() override;
    bool isRunning() override { return llfIn!=nullptr; }
    bool isReceivingData() override { return gotDataFromFetch; }
    std::vector<std::string> getOptionNames() override { return {}; }
};
}
