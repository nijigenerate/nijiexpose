#pragma once
#include "adaptor.hpp"
#include "osc/server.hpp"
namespace nijiexpose::ft {
class PhizOSCAdaptor final: public Adaptor {
    std::unique_ptr<osc::Server> server;
    uint16_t port=41235;
    std::string bind="0.0.0.0";
    bool gotDataFromFetch=false;
public:
    ~PhizOSCAdaptor() override { stop(); }
    std::string getAdaptorName() override { return "Phiz OSC Receiver"; }
    void start() override;
    void stop() override;
    void poll() override;
    bool isRunning() override { return server!=nullptr; }
    bool isReceivingData() override { return gotDataFromFetch; }
    std::vector<std::string> getOptionNames() override { return {"port","address"}; }
};
}
