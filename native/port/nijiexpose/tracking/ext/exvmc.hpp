#pragma once
#include "ft/adaptor.hpp"
#include "osc/server.hpp"
namespace nijiexpose::tracking {
class ExVMCAdaptor final:public ft::Adaptor {
    std::unique_ptr<osc::Server> server;
    uint16_t port=39540;
    std::string bind="0.0.0.0";
    bool gotDataFromFetch=false;
public:
    ~ExVMCAdaptor() override { if(isRunning()) stop(); }
    std::string getAdaptorName() override { return "VMC Receiver"; }
    void start() override;
    bool isRunning() override { return server!=nullptr; }
    void stop() override;
    void poll() override;
    bool isReceivingData() override { return gotDataFromFetch; }
    std::vector<std::string> getOptionNames() override { return {"port","address"}; }
};
bool neIsEventOn(int keyCode);
}
