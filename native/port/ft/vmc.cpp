// Direct translation of facetrack-d 0.8.0 ft/adaptors/vmc.d.
#include "vmc.hpp"
#include <charconv>
namespace nijiexpose::ft {
void VMCAdaptor::start() {
    if(options.contains("port")) {
        const auto& text=options.at("port"); unsigned value=0;
        auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
        if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||value>65535) throw std::runtime_error("Invalid tracking port");
        port=static_cast<uint16_t>(value);
    }
    if(options.contains("address")) bind=options.at("address");
    server=std::make_unique<osc::Server>(bind,port);
}
void VMCAdaptor::stop() { if(server) { server->close(); server.reset(); } }
void VMCAdaptor::poll() {
    if(!isRunning()) return;
    auto msgs=server->popMessages();
    if(!msgs.empty()) {
        dataLossCounter=0; gotDataFromFetch=true;
        for(const auto& msg:msgs) {
            const auto& path=msg.addressPattern;
            if(path.size()<3) continue;
            if(path[0]!="/VMC" && path[1]!="/Ext") continue;
            if(path[2]=="/Bone") {
                if(path.size()<4) continue;
                if(path[3]!="/Pos") continue;
                if(path.size()>4) {
                    const auto& pattern=path[4];
                    if(pattern.size()>1) {
                        if(msg.typeTags.size()!=7) continue;
                        // Original pattern[$..1] is an invalid slice for this branch.
                        throw std::out_of_range("D VMC bone pattern[$..1] range");
                    }
                } else {
                    if(msg.typeTags.size()!=8) continue;
                    auto boneName=msg.arg<std::string>(0);
                    if(!bones.contains(boneName)) {
                        float nan=std::numeric_limits<float>::quiet_NaN(); bones[boneName]={{nan,nan,nan},{}};
                    }
                    bones[boneName].position={msg.arg<float>(1),msg.arg<float>(2),msg.arg<float>(3)};
                    bones[boneName].rotation={msg.arg<float>(7),-msg.arg<float>(6),msg.arg<float>(4),-msg.arg<float>(5)};
                }
            } else if(path[2]=="/Blend") {
                if(path.size()>3) {
                    auto pattern=path[3];
                    if(pattern=="/Val") {
                        if(msg.typeTags.size()==2) {
                            if(!msg.arg<std::string>(0).empty()) blendshapes[msg.arg<std::string>(0)]=msg.arg<float>(1);
                        } else if(msg.typeTags.size()==1) {
                            if(path.size()!=4) continue;
                            pattern=path.at(4); // Original checks length==4 before reading element 4.
                            if(pattern.size()>1) blendshapes[pattern.substr(1)]=msg.arg<float>(0);
                        }
                    }
                }

            }
        }
    } else { ++dataLossCounter; if(dataLossCounter>RECV_TIMEOUT) gotDataFromFetch=false; }
}
}
