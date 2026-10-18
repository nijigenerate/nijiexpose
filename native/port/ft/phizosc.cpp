// facetrack-d 0.8.0 ft.adaptors.phizosc, preserving message and update order.
#include "phizosc.hpp"
#include <charconv>
#include "phizosc_names.hpp"
namespace nijiexpose::ft {
void PhizOSCAdaptor::start() {
    if(options.contains("port")) {
        const auto& text=options.at("port"); unsigned value=0;
        auto result=std::from_chars(text.data(),text.data()+text.size(),value);
        if(result.ec!=std::errc{} || result.ptr!=text.data()+text.size() || value>65535) throw std::runtime_error("Invalid tracking port");
        port=static_cast<uint16_t>(value);
    }
    if(options.contains("address")) bind=options.at("address");
    server=std::make_unique<osc::Server>(bind,port);
}
void PhizOSCAdaptor::stop() { if(server) { server->close(); server.reset(); } }
void PhizOSCAdaptor::poll() {
    if(!isRunning()) return;
    const auto msgs=server->popMessages();
    if(!msgs.empty()) {
        dataLossCounter=0; gotDataFromFetch=true;
        for(const auto& msg:msgs) {
            if(msg.addressPattern.size()<2 || msg.addressPattern[0]!="/phiz") continue;
            auto& path=msg.addressPattern[1];
            const char* bone=path=="/headRotation"?"Head":path=="/leftEyeRotation"?"LeftGaze":path=="/rightEyeRotation"?"RightGaze":nullptr;
            if(bone) {
                // The original looks up the first argument as raw string bytes.
                if(!bones.contains(msg.arg<std::string>(0))) {
                    float nan=std::numeric_limits<float>::quiet_NaN();
                    bones[bone]={{nan,nan,nan},{}};
                }
                bones.at(bone).rotation={msg.arg<float>(3),-msg.arg<float>(2),msg.arg<float>(0),-msg.arg<float>(1)};
            } else if(path=="/blendshapes") {
                size_t i=0;
                for(auto name:phizOSCBlendshapes) blendshapes[name]=msg.arg<float>(i++);
            }
        }
    } else { ++dataLossCounter; if(dataLossCounter>RECV_TIMEOUT) gotDataFromFetch=false; }
}
}
