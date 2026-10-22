#include "tracking/ext/exvmc.hpp"
#include "ft/phizosc.hpp"
#include "ft/llf.hpp"
#include "ft/phone.hpp"
#include "ft/openseeface.hpp"
#include "ft/vtsproto.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <bit>
using namespace nijiexpose;
int main(int argc,char** argv) {
    if(argc!=3) return 2;
    std::unique_ptr<ft::Adaptor> a;
    if(std::string(argv[1])=="vmc") a=std::make_unique<tracking::ExVMCAdaptor>();
    else if(std::string(argv[1])=="llf") a=std::make_unique<ft::LLFAdaptor>();
    else if(std::string(argv[1])=="ifm") a=std::make_unique<ft::IFMAdaptor>();
    else if(std::string(argv[1])=="fm3d") a=std::make_unique<ft::FM3DAdaptor>();
    else if(std::string(argv[1])=="osf") a=std::make_unique<ft::OSFAdaptor>();
    else if(std::string(argv[1])=="vts") a=std::make_unique<ft::VTSAdaptor>();
    else a=std::make_unique<ft::PhizOSCAdaptor>();
    // Broadcast send is rejected by Winsock without SO_BROADCAST: no remote
    // traffic and no request packets looping into the local receive fixture.
    a->setOptions({{"port",argv[2]},{"address","localhost"},{"phoneIP",std::string(argv[1])=="vts"?"255.255.255.255":"127.0.0.1"},{"osf_bind_port",argv[2]},{"osf_bind_ip","localhost"}}); a->start();
    std::cout<<"READY"<<std::endl;
    for(std::string command;std::getline(std::cin,command);) {
        if(command=="quit") break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        int count=std::stoi(command);
        for(int i=0;i<count;++i) a->poll();
        std::cout<<"state "<<a->isRunning()<<' '<<a->isReceivingData()<<' '<<tracking::neIsEventOn('a')<<' '<<tracking::neIsEventOn(0xE9)<<'\n';
        for(const auto& [key,value]:a->getBlendshapes()) std::cout<<"blend "<<key<<' '<<std::bit_cast<uint32_t>(value)<<'\n';
        for(const auto& [key,b]:a->getBones()) {
            std::cout<<"bone "<<key;
            for(float v:{b.position.x,b.position.y,b.position.z,b.rotation.w,b.rotation.x,b.rotation.y,b.rotation.z}) std::cout<<' '<<std::bit_cast<uint32_t>(v);
            std::cout<<'\n';
        }
        std::cout<<"END"<<std::endl;
    }
    a->stop();
    std::cout<<"STOPPED"<<std::endl;
}
