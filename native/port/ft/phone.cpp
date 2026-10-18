// facetrack-d 0.8.0 ft.adaptors.ifacialmocap and ft.adaptors.facemotion3d.
#include "phone.hpp"
#include "quaternion.hpp"
#include <array>
#include <chrono>
#include <charconv>
#include <iostream>
namespace nijiexpose::ft {
namespace {
float number(std::string_view text) {
    float result;
    if(!text.empty() && text.front()=='+') text.remove_prefix(1);
    auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
    if(parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size()) throw std::runtime_error("Invalid tracking number: "+std::string(text));
    return result;
}
}
PhoneTrackingData::PhoneTrackingData(std::string datastr,char delimiter):indata(datastr) {
    size_t i=0,nBufStart=0,nBufEnd=0,nBufSplit=0;
    try {
        while(i<datastr.size()) {
            while(datastr.at(i)==' ') ++i;
            if(datastr.at(i)!='|') nBufEnd=i;
            else {
                ++nBufEnd;
                if(datastr.at(nBufStart)=='=') {
                    nBufStart+=5;
                    std::array<float,6> values; values.fill(std::numeric_limits<float>::quiet_NaN());
                    size_t rStart=nBufStart+1,rEnd=rStart,aIdx=0;
                    while(rEnd<nBufEnd) {
                        ++rEnd;
                        if(datastr.at(rEnd)==',' || datastr.at(rEnd)=='|') { values.at(aIdx++)=number(std::string_view(datastr).substr(rStart,rEnd-rStart)); rStart=rEnd+1; }
                    }
                    headRot={values[0],values[1],values[2]}; headPos={values[3],values[4],values[5]};
                } else {
                    nBufSplit=nBufStart;
                    while(nBufSplit++<nBufEnd) {
                        if(datastr.at(nBufSplit)==delimiter) {
                            blendshapes[datastr.substr(nBufStart,nBufSplit-nBufStart)]=static_cast<float>(number(std::string_view(datastr).substr(nBufSplit+1,nBufEnd-nBufSplit-1))/100.0);
                            break;
                        } else if(datastr.at(nBufSplit)=='#') {
                            std::array<float,3> values; values.fill(std::numeric_limits<float>::quiet_NaN());
                            size_t rStart=nBufSplit+1,rEnd=rStart,aIdx=0;
                            while(rEnd<nBufEnd) {
                                ++rEnd;
                                if(datastr.at(rEnd)==',' || datastr.at(rEnd)=='|') { values.at(aIdx++)=number(std::string_view(datastr).substr(rStart,rEnd-rStart)); rStart=rEnd+1; }
                            }
                            const auto name=datastr.substr(nBufStart,nBufSplit-nBufStart);
                            if(name=="leftEye") leftEye={values[0],values[1],values[2]};
                            else if(name=="rightEye") rightEye={values[0],values[1],values[2]};
                            break;
                        }
                    }
                }
                nBufStart=++i; nBufEnd=nBufStart;
            }
            ++i;
        }
    } catch(const std::runtime_error& ex) {
        std::cout<<ex.what()<<' '<<nBufStart<<' '<<nBufSplit<<' '<<nBufEnd<<' '<<datastr.substr(nBufStart,nBufEnd-nBufStart)<<'\n';
    }
}
void PhoneAdaptor::request() {
    std::string text=fm3d?"FACEMOTION3D_OtherStreaming":"iFacialMocap_sahuasouryya9218sauhuiayeta91555dy3719";
    sender->sendTo({reinterpret_cast<const uint8_t*>(text.data()),text.size()},phoneIP,fm3d?49993:49983);
}
void PhoneAdaptor::listenThread() {
    std::array<uint8_t,65535> buff{};
    int failed=0;
    while(!isCloseRequested) {
        try {
            auto recvBytes=dataIn->receive(buff);
            if(recvBytes!=-1 && recvBytes<=static_cast<int>(buff.size())) {
                ++dataPacketsReceivedTotal;
                PhoneTrackingData trackingData(std::string(reinterpret_cast<char*>(buff.data()),recvBytes),fm3d?'&':'-');
                failed=0;
                std::lock_guard lock(mutex); updated_=true; data=std::move(trackingData);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        } catch(const std::runtime_error& ex) {
            std::cout<<ex.what()<<'\n'; ++failed;
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
            if(failed>100) {
                request(); std::this_thread::sleep_for(std::chrono::seconds(1)); failed=0; dataLossCounter=RECV_TIMEOUT;
            }
        }
    }
}
void PhoneAdaptor::start() {
    if(options.contains("phoneIP")) phoneIP=options.at("phoneIP"); else return;
    if(isRunning()) stop();
    isCloseRequested=false;
    { std::lock_guard lock(mutex); updated_=false; data={}; }
    try {
        sender=std::make_unique<UdpSocket>(); request();
        dataIn=std::make_unique<UdpSocket>(); dataIn->timeout(5); dataIn->bind("0.0.0.0",49983);
    } catch(const std::runtime_error&) {
        if(!dataIn) throw; // D calls close on null in this failure branch.
        dataIn->close(); dataIn.reset(); return;
    }
    dataPacketsReceivedTotal=0;
    listeningThread=std::thread([this]{listenThread();});
}
void PhoneAdaptor::stop() {
    if(isRunning()) {
        isCloseRequested=true;
        if(listeningThread.joinable()) listeningThread.join();
        dataIn->close(); dataIn.reset();
    }
}
void PhoneAdaptor::poll() {
    if(!isRunning()) return;
    bool updated; { std::lock_guard lock(mutex); updated=updated_; }
    if(updated) {
        PhoneTrackingData trackingData;
        { std::lock_guard lock(mutex); updated_=false; trackingData=data; }
        dataLossCounter=0; gotDataFromFetch=true;
        auto radians=[](float degrees) { return (std::numbers::pi/180)*degrees; };
        auto rotation=[&](Vec3 angles) { return eulerRotation(radians(angles.z),radians(angles.y),radians(angles.x)); };
        bones["Head"]={{trackingData.headPos.x*-1,trackingData.headPos.y,trackingData.headPos.z},rotation(trackingData.headRot)};
        bones["LeftEye"]={{0,0,0},rotation(trackingData.leftEye)};
        bones["RightEye"]={{trackingData.headPos.x*-1,trackingData.headPos.y,trackingData.headPos.z},rotation(trackingData.rightEye)};
        blendshapes=trackingData.blendshapes;
        if(!blendshapes.empty()) {
            auto get=[&](const char* fm,const char* ifm) { return blendshapes.at(fm3d?fm:ifm); };
            blendshapes["ftEyeBlinkLeft"]=get("eyeBlinkLeft","eyeBlink_L");
            blendshapes["ftEyeXLeft"]=get("eyeLookOutLeft","eyeLookOut_L")-get("eyeLookInLeft","eyeLookIn_L");
            blendshapes["ftEyeYLeft"]=get("eyeLookUpLeft","eyeLookUp_L")-get("eyeLookDownLeft","eyeLookDown_L");
            blendshapes["ftEyeSquintLeft"]=get("eyeSquintLeft","eyeSquint_L");
            blendshapes["ftEyeWidenLeft"]=get("eyeWideLeft","eyeWide_L");
            blendshapes["ftEyeBlinkRight"]=get("eyeBlinkRight","eyeBlink_R");
            blendshapes["ftEyeXRight"]=get("eyeLookInRight","eyeLookIn_R")-get("eyeLookOutRight","eyeLookOut_R");
            blendshapes["ftEyeYRight"]=get("eyeLookUpRight","eyeLookUp_R")-get("eyeLookDownRight","eyeLookDown_R");
            blendshapes["ftEyeSquintRight"]=get("eyeSquintRight","eyeSquint_R");
            blendshapes["ftEyeWidenRight"]=get("eyeWideRight","eyeWide_R");
            blendshapes["ftMouthOpen"]=std::clamp((get("mouthLowerDownLeft","mouthLowerDown_L")+get("mouthUpperUpLeft","mouthUpperUp_L"))/2+(get("mouthLowerDownRight","mouthLowerDown_R")+get("mouthUpperUpRight","mouthUpperUp_R"))/2,0.f,1.f);
            blendshapes["ftMouthX"]=static_cast<float>((1+get("mouthLeft","mouthLeft")-get("mouthRight","mouthRight"))/2.0);
            blendshapes["ftMouthEmotion"]=static_cast<float>(std::clamp(1+(get("mouthSmileLeft","mouthSmile_L")+get("mouthSmileRight","mouthSmile_R")/2.0)-(get("mouthFrownLeft","mouthFrown_L")+get("mouthFrownRight","mouthFrown_R")/2.0),0.0,2.0)/2.0);
        }
    } else { if(dataLossCounter>RECV_TIMEOUT*10) gotDataFromFetch=false; ++dataLossCounter; }
}
}
