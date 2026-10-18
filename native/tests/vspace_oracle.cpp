#include "nijiexpose/tracking/vspace.hpp"
#include "ft/quaternion.hpp"
#include <iostream>
#include <bit>
using namespace nijiexpose;
struct Source:ft::Adaptor {
    bool active=false; int polls=0;
    void start() override {} void stop() override {} void poll() override {++polls;}
    std::vector<std::string> getOptionNames() override {return {};}
    bool isRunning() override {return true;} bool isReceivingData() override {return active;}
    std::string getAdaptorName() override {return "Fixture";}
};
int main() {
    auto a=std::make_shared<Source>(),b=std::make_shared<Source>(),c=std::make_shared<Source>();
    tracking::VirtualSpace space;
    auto za=std::make_shared<tracking::VirtualSpaceZone>("a"),zb=std::make_shared<tracking::VirtualSpaceZone>("b"),zc=std::make_shared<tracking::VirtualSpaceZone>("c");
    za->sources={a,b,nullptr}; zb->sources={b}; zc->sources={c};
    space.addZone(za); space.addZone(zb); space.addZone(zc);
    for(int step=0;step<7;++step) {
        if(step==1){b->active=true;c->active=true;}
        if(step==3){c->active=false;}
        if(step==5){space.removeZone(zb);}
        space.update();
        std::cout<<"focus "<<space.currentZone()->name<<' '<<space.hasAnyFocus()<<' '<<a->polls<<' '<<b->polls<<' '<<c->polls<<'\n';
    }
    a->getBlendshapes()["x"]=0.25; b->getBlendshapes()["x"]=0.75;
    std::cout<<"blend "<<za->getBlendshapeFor("x")<<' '<<za->getBlendshapeFor("absent")<<'\n';
    for(int i=0;i<20;++i) {
        a->getBones()["head"]={{1,2,3},ft::eulerRotation(i*0.05,i*0.02,i*-0.01)};
        b->getBones()["head"]={{-1,4,7},ft::eulerRotation(i*-0.03,i*0.07,i*0.04)};
        auto bone=za->getBoneFor("head");
        std::cout<<"bone";
        for(float v:{bone.position.x,bone.position.y,bone.position.z,bone.rotation.w,bone.rotation.x,bone.rotation.y,bone.rotation.z}) std::cout<<' '<<std::bit_cast<uint32_t>(v);
        std::cout<<'\n';
    }
}
