#include "osc/server.hpp"
#include "ft/phizosc.hpp"
#include "nijiexpose/tracking/ext/exvmc.hpp"
#include <chrono>
#include <iostream>
#include <thread>
int main(int argc,char** argv) {
    if(argc==3) {
        std::unique_ptr<nijiexpose::ft::Adaptor> adaptor;
        if(std::string(argv[2])=="vmc")
            adaptor=std::make_unique<nijiexpose::tracking::ExVMCAdaptor>();
        else adaptor=std::make_unique<nijiexpose::ft::PhizOSCAdaptor>();
        adaptor->start({{"address","127.0.0.1"},{"port",argv[1]}});
        std::cout<<"ready"<<std::endl;
        auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
        while(std::chrono::steady_clock::now()<deadline) {
            adaptor->poll();
            if(adaptor->getBlendshapes().contains("alive") ||
               adaptor->getBones().contains("Head")) {
                std::cout<<"alive"<<std::endl;return 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return 1;
    }
    if(argc!=2)return 2;
    nijiexpose::osc::Server server("127.0.0.1",static_cast<uint16_t>(std::stoi(argv[1])));
    std::cout<<"ready"<<std::endl;
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    while(std::chrono::steady_clock::now()<deadline) {
        for(auto& message:server.popMessages())
            if(message.addressPattern==std::vector<std::string>{"/alive"} &&
               message.typeTags=="i" && message.arg<int>(0)==123) {
                std::cout<<"alive"<<std::endl;return 0;
            }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return 1;
}
