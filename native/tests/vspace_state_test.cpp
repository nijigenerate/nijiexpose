#include "tracking/vspace.hpp"
#include "tracking/ext/package.hpp"
#include "ft/factory.hpp"
#include "nijiui/core/settings.hpp"
#include <iostream>
using namespace nijiexpose;
namespace {
void require(bool value,const char* name) {if(!value)throw std::runtime_error(name);}
struct Fixture:ft::Adaptor {
    int starts=0,polls=0;bool running=false;
    void start() override {++starts;if(options.contains("fail"))throw std::runtime_error("Fixture start failure");running=true;}
    void stop() override {running=false;}void poll() override {++polls;}
    bool isRunning() override {return running;}bool isReceivingData() override {return running;}
    std::string getAdaptorName() override {return "Fixture";}
    std::vector<std::string> getOptionNames() override {return {};}
};
}
int main() {
    for(const char* name:{"VTubeStudio","OpenSeeFace","VMC Receiver","Phiz OSC Receiver","iFacialMocap","Facemotion3D","LiveLinkFace/MeFaMo Receiver"}) {
        auto a=tracking::neCreateAdaptor(name);require(a && a->getAdaptorName()==name,"standard factory");require(!a->isRunning(),"factory must not start without options");
    }
    require(!tracking::neCreateAdaptor("unknown"),"unknown factory");
    require(!tracking::neCreateAdaptor("VMC Receiver",std::map<std::string,std::string>{{"port","invalid"}})->isRunning(),"ExVMC ignores factory options");
    ft::ftRegisterAdaptorFactory("Fixture",[]{return std::make_shared<Fixture>();});
    ft::ftRegisterAdaptorFactory("Phiz OSC Receiver",[]{return std::make_shared<Fixture>();});
    auto space=std::make_shared<tracking::VirtualSpace>();
    space->update();require(!space->hasAnyFocus(),"empty tracking configuration must not overwrite model defaults with zero input");
    const auto data=json::parse(R"({"zones":[{"name":"zone","sources":[{"type":"Fixture"},{"type":"Fixture","options":{"appName":"wrong"}},{"type":"Fixture","options":{"fail":"yes"}},{"type":"Phiz OSC Receiver","options":{"address":"192.0.2.1"}},{"type":"unknown"}]}]})");
    space->deserializeFromJson(data);
    auto zone=space->currentZone(); require(zone->sources.size()==4,"source append order and failed start retention");
    auto first=std::dynamic_pointer_cast<Fixture>(zone->sources[0]);
    auto second=std::dynamic_pointer_cast<Fixture>(zone->sources[1]);
    auto failed=std::dynamic_pointer_cast<Fixture>(zone->sources[2]);
    require(first->starts==0 && second->starts==1 && failed->starts==1,"options control start");
    require(second->getOptions().at("appName")=="nijiexpose","forced app name");
    require(zone->sources[3]->getOptions().at("address")=="0.0.0.0","forced receiver bind");
    require(first->polls==1 && second->polls==1 && failed->polls==1,"rebuild polls every source");
    space->update();require(!space->hasAnyFocus(),"only first source controls focus");
    auto encoded=space->serialize();require(encoded.at("zones").as_array().size()==1,"serialize zones");
    nijiui::inSettingsLoad();tracking::insSaveVSpace(space);nijiui::inSettingsLoad();
    auto loaded=tracking::insLoadVSpace();require(loaded->getZones().size()==1 && loaded->currentZone()->sources.size()==4,"settings round trip");
    // Saving writes the forced options even for the previously unstarted source;
    // the original therefore starts that source on the next load.
    require(loaded->currentZone()->sources[0]->isRunning(),"saved options start on reload");
    loaded->deserializeFromJson(encoded);require(loaded->getZones().size()==2,"deserialize appends zones");
    std::cout<<"VirtualSpace factories, start order, failure retention, focus and settings round trip passed\n";
}
