// Factory operations from ft.adaptors. Additional default adaptor ports are
// registered here when their implementations are linked into the application.
#include "factory.hpp"
#include "phizosc.hpp"
#include "llf.hpp"
#include "phone.hpp"
#include "openseeface.hpp"
#include "vtsproto.hpp"
#include "vmc.hpp"
namespace nijiexpose::ft {
namespace {
thread_local std::map<std::string,AdaptorFactory> adaptorFactories{
    {"VMC Receiver",[]{return std::make_shared<VMCAdaptor>();}},
    {"Phiz OSC Receiver",[]{return std::make_shared<PhizOSCAdaptor>();}},
    {"LiveLinkFace/MeFaMo Receiver",[]{return std::make_shared<LLFAdaptor>();}},
    {"iFacialMocap",[]{return std::make_shared<IFMAdaptor>();}},
    {"Facemotion3D",[]{return std::make_shared<FM3DAdaptor>();}},
    {"OpenSeeFace",[]{return std::make_shared<OSFAdaptor>();}},
    {"VTubeStudio",[]{return std::make_shared<VTSAdaptor>();}}
};
}
void ftRegisterAdaptorFactory(const std::string& name,AdaptorFactory factory) { adaptorFactories[name]=std::move(factory); }
std::shared_ptr<Adaptor> ftCreateAdaptor(const std::string& name) {
    auto found=adaptorFactories.find(name); return found==adaptorFactories.end()?nullptr:found->second();
}
std::shared_ptr<Adaptor> ftCreateAdaptor(const std::string& name,std::map<std::string,std::string> options) {
    auto adaptor=ftCreateAdaptor(name); if(adaptor) adaptor->start(std::move(options)); return adaptor;
}
}
