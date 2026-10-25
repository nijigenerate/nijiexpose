// Direct port of legacy D: nijiexpose/tracking/ext/exvmc.d.
#include "exvmc.hpp"
#include <charconv>
#include <set>
#include "dcompat/uppercase_table.hpp"
#include <algorithm>
namespace nijiexpose::tracking {
namespace {
thread_local std::set<int> keyStatus;
thread_local std::set<int> embeddedKeyStatus;
int uppercase(int code) {
    uint32_t c=static_cast<uint32_t>(code);
    if(c<0xAA) { if(c<'a') return code; if(c<='z') return code-32; return code; }
    auto& table=dcompat::uppercaseMap;
    auto it=std::lower_bound(std::begin(table),std::end(table),c,[](const auto& entry,uint32_t value){return entry.from<value;});
    return it!=std::end(table)&&it->from==c ? static_cast<int>(it->to) : code;
}
}
void ExVMCAdaptor::start() {
    if(options.contains("port")) {
        const auto& text=options.at("port"); unsigned value=0;
        auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
        if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||value>65535) throw std::runtime_error("Invalid tracking port");
        port=static_cast<uint16_t>(value);
    }
    if(options.contains("address")) bind=options.at("address");
    server=std::make_unique<osc::Server>(bind,port);
}
void ExVMCAdaptor::stop() { if(server) { server->close(); server.reset(); } }
void ExVMCAdaptor::poll() {
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
                        // This unsupported form previously threw on the UI thread.
                        // Reject it at the untrusted input boundary.
                        continue;
                    }
                } else {
                    if(msg.typeTags!="sfffffff") continue;
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
                        if(msg.typeTags=="sf") {
                            if(!msg.arg<std::string>(0).empty()) blendshapes[msg.arg<std::string>(0)]=msg.arg<float>(1);
                        } else if(msg.typeTags.size()==1) {
                            // The original branch indexed beyond the path.
                            // Do not let an unsupported network message throw.
                            continue;
                        }
                    }
                }
            } else if(path[2]=="/Key") {
                if(msg.typeTags!="isi")continue;
                auto active=msg.arg<int>(0);
                [[maybe_unused]] auto name=msg.arg<std::string>(1);
                auto keyCode=msg.arg<int>(2);
                if(active) keyStatus.insert(uppercase(keyCode)); else keyStatus.erase(uppercase(keyCode));
            }
        }
    } else { ++dataLossCounter; if(dataLossCounter>RECV_TIMEOUT) gotDataFromFetch=false; }
}
bool neIsEventOn(int keyCode) { return keyStatus.contains(uppercase(keyCode)) || embeddedKeyStatus.contains(uppercase(keyCode)); }
void neSetEmbeddedKey(int keyCode,bool pressed){if(pressed)embeddedKeyStatus.insert(uppercase(keyCode));else embeddedKeyStatus.erase(uppercase(keyCode));}
void neClearEmbeddedKeys(){embeddedKeyStatus.clear();}
}
