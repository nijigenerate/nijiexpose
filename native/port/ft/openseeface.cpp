// facetrack-d 0.8.0 ft.adaptors.openseeface.
#include "openseeface.hpp"
#include "quaternion.hpp"
#include <bit>
#include <charconv>
#include <chrono>
#include <cstring>
namespace nijiexpose::ft {
namespace {
constexpr const char* features[]={"eyeLeft","eyeRight","eyebrowSteppnessLeft","eyebrowUpDownLeft","eyebrowQuirkLeft","eyebrowSteppnessRight","eyebrowUpDownRight","eyebrowQuirkRight","mouthCornerUpDownLeft","mouthCornerInOutLeft","mouthCornerUpDownRight","mouthCornerInOutRight","mouthOpen","mouthWide"};
struct Reader {
    const uint8_t* bytes;
    template<class T> T read() { T value; std::memcpy(&value,bytes,sizeof(T)); bytes+=sizeof(T); return value; }
    Vec3 vec() { float x=read<float>(),y=read<float>(),z=read<float>(); return {x,y,z}; }
};
}
void OSFAdaptor::receiveThread() {
    constexpr size_t packetFrameSize=8+4+2*4+2*4+1+4+3*4+3*4+4*4+4*68+4*2*68+4*3*70+4*14;
    std::array<uint8_t,packetFrameSize> buffer{};
    while(!isCloseRequested) {
        try {
            int recvBytes=osf->receive(buffer); if(recvBytes<static_cast<int>(packetFrameSize)) continue;
            Reader bytes{buffer.data()}; OSFData next;
            next.time=bytes.read<double>(); next.id=bytes.read<int>(); next.cameraResolution={bytes.read<float>(),bytes.read<float>()};
            next.rightEyeOpen=bytes.read<float>(); next.leftEyeOpen=bytes.read<float>();
            next.got3dPoints=bytes.read<uint8_t>()!=0; next.fit3dError=bytes.read<float>();
            float qx=bytes.read<float>(),qy=bytes.read<float>(),qz=bytes.read<float>(),qw=bytes.read<float>();
            next.rawQuaternion={-qw,qx,qy,qz}; next.rawEuler=bytes.vec(); next.translation=bytes.vec();
            for(auto& v:next.confidence) v=bytes.read<float>();
            for(auto& v:next.points) v={bytes.read<float>(),bytes.read<float>()};
            for(auto& v:next.points3d) {v=bytes.vec();v.y=-v.y;}
            auto direction=[](Vec3 a,Vec3 b)->Vec3 {return {-a.x-(-b.x),a.y-b.y,a.z-b.z};};
            next.rightGaze=lookRotation(direction(next.points3d[66],next.points3d[68]),{0,1,0})*axisRotation(std::numbers::pi,{1,0,0})*axisRotation(std::numbers::pi,{0,0,1});
            next.leftGaze=lookRotation(direction(next.points3d[67],next.points3d[69]),{0,1,0})*axisRotation(std::numbers::pi,{1,0,0})*axisRotation(std::numbers::pi,{0,0,1});
            for(auto name:features) next.features[name]=bytes.read<float>();
            std::lock_guard lock(mutex); updated_=true; data=std::move(next);
        } catch(const std::runtime_error&) {std::this_thread::sleep_for(std::chrono::milliseconds(100));}
    }
}
void OSFAdaptor::start() {
    if(options.contains("osf_bind_port")) {
        const auto& text=options.at("osf_bind_port"); unsigned value;
        auto result=std::from_chars(text.data(),text.data()+text.size(),value);
        if(result.ec!=std::errc{} || result.ptr!=text.data()+text.size() || value>65535) throw std::runtime_error("Invalid OSF port");
        port=static_cast<uint16_t>(value);
    }
    if(options.contains("osf_bind_ip")) bind=options.at("osf_bind_ip");
    if(isRunning()) stop();
    isCloseRequested=false;
    {std::lock_guard lock(mutex);updated_=false;data={};}
    osf=std::make_unique<UdpSocket>(); osf->bind(bind,port); osf->timeout(16);
    receivingThread=std::thread([this]{receiveThread();});
}
void OSFAdaptor::stop() {
    if(isRunning()) {isCloseRequested=true;if(receivingThread.joinable())receivingThread.join();osf->close();osf.reset();}
}
void OSFAdaptor::poll() {
    if(!isRunning()) return;
    bool updated; {std::lock_guard lock(mutex);updated=updated_;}
    if(updated) {
        dataLossCounter=0;gotDataFromFetch=true;
        OSFData next; {std::lock_guard lock(mutex);updated_=false;next=data;}
        if(next.got3dPoints) {
            auto toRotate=eulerRotation((std::numbers::pi/180)*180,0,(std::numbers::pi/180)*90);
            auto temp=toRotate*next.rawQuaternion;
            bones["Head"]={next.translation,{temp.w,temp.z,temp.x,temp.y}};
            bones["LeftGaze"]={{0,0,0},{next.leftGaze.w,next.leftGaze.z,next.leftGaze.x,next.leftGaze.y}};
            bones["RightGaze"]={{0,0,0},{next.rightGaze.w,next.rightGaze.z,next.rightGaze.x,next.rightGaze.y}};
            blendshapes=next.features;
            blendshapes["EyeOpenRight"]=next.rightEyeOpen;blendshapes["EyeOpenLeft"]=next.leftEyeOpen;
            blendshapes["ftEyeBlinkLeft"]=1-next.leftEyeOpen;blendshapes["ftEyeBlinkRight"]=1-next.rightEyeOpen;
            blendshapes["ftMouthOpen"]=next.features.at("mouthOpen");
            blendshapes["ftMouthX"]=static_cast<float>((1+next.features.at("mouthCornerInOutLeft")-next.features.at("mouthCornerInOutRight"))/2.0);
            blendshapes["ftMouthEmotion"]=static_cast<float>(std::clamp(1+((next.features.at("mouthCornerUpDownRight")*2)-1)-((next.features.at("mouthCornerUpDownLeft")*2)-1),0.f,2.f)/2.0);
        }
    } else {++dataLossCounter;if(dataLossCounter>RECV_TIMEOUT)gotDataFromFetch=false;}
}
}
