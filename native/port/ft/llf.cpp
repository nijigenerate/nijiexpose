// facetrack-d 0.8.0 ft.adaptors.llf.
#include "llf.hpp"
#include "llf_names.hpp"
#include <bit>
#include <chrono>
namespace nijiexpose::ft {
void LLFAdaptor::listenThread() {
    std::array<uint8_t,65535> buff{};
    while(!isCloseRequested) {
        try {
            int recvBytes=llfIn->receive(buff);
            if(recvBytes!=-1 && recvBytes<=static_cast<int>(buff.size())) {
                if(recvBytes<46) continue;
                uint32_t nameLen=(uint32_t(buff[43])<<8)|buff[44];
                if(static_cast<uint32_t>(recvBytes)<45+nameLen+17) continue;
                size_t offset=45+nameLen+17;
                std::array<float,61> trackingData{};
                for(size_t i=0;i<llfBlendshapeNames.size();++i) {
                    if(static_cast<size_t>(recvBytes)-offset>=4) {
                        uint32_t bits=(uint32_t(buff[offset])<<24)|(uint32_t(buff[offset+1])<<16)|(uint32_t(buff[offset+2])<<8)|buff[offset+3];
                        trackingData[i]=std::bit_cast<float>(bits); offset+=4;
                    } else trackingData[i]=0.f;
                }
                std::lock_guard lock(mutex); updated_=true; data=trackingData;
            }
        } catch(const std::exception&) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
    }
}
void LLFAdaptor::start() {
    if(isRunning()) stop();
    isCloseRequested=false;
    { std::lock_guard lock(mutex); updated_=false; data.fill(std::numeric_limits<float>::quiet_NaN()); }
    llfIn=std::make_unique<UdpSocket>(); llfIn->timeout(16); llfIn->bind("0.0.0.0",11111);
    listeningThread=std::thread([this]{listenThread();});
}
void LLFAdaptor::stop() {
    if(isRunning()) {
        isCloseRequested=true;
        if(listeningThread.joinable()) listeningThread.join();
        llfIn->close(); llfIn.reset();
    }
}
void LLFAdaptor::poll() {
    if(!isRunning()) return;
    bool updated;
    { std::lock_guard lock(mutex); updated=updated_; }
    if(updated) {
        std::array<float,61> trackingData;
        { std::lock_guard lock(mutex); updated_=false; trackingData=data; }
        dataLossCounter=0; gotDataFromFetch=!trackingData.empty();
        for(size_t i=0;i<llfBlendshapeNames.size();++i) blendshapes[llfBlendshapeNames[i]]=i<trackingData.size()?trackingData[i]:0;
    } else { ++dataLossCounter; if(dataLossCounter>RECV_TIMEOUT) gotDataFromFetch=false; }
}
}
