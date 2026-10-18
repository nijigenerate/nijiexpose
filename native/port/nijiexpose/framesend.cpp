// Windows port of source/nijiexpose/framesend.d.
#include "framesend.hpp"
#include "log.hpp"
#include "i18n/tr.hpp"
#include "nijilive/opengl/runtime.hpp"
#include "spout.hpp"
#include <core/runtime_state.hpp>
namespace nijiexpose {
namespace {
thread_local bool loadSuccessful=false;
thread_local spout::SPOUTHANDLE spHandle=nullptr;
}
void insInitFrameSending() {
    using namespace spout;
    auto loadMode=loadSpout2();
    loadSuccessful=loadMode==Spout2Support::spout2;
    if(loadSuccessful) {
        std::string senderName="nijiexpose";
        spHandle=spGetSpout();
        int i=1;
        while(spFindSenderName(spHandle,senderName.data())) senderName="nijiexpose ("+std::to_string(i)+")";
        spSetSenderName(spHandle,senderName.c_str());
        spSetSenderFormat(spHandle,28);
    }
    if(!loadSuccessful) insLogErr(i18n::_("Could not start texture sharing, it will be disabled. Is the library missing?"));
    else insLogInfo(i18n::_("Frame-sending started successfully!"));
}
void insCleanupFrameSending() {
    if(loadSuccessful) { spout::spReleaseSender(spHandle,0); spHandle=nullptr; spout::unloadSpout2(); }
}
void insSendFrame() {
    if(loadSuccessful) {
        int w,h; nicxlive::core::inGetViewport(w,h);
        if(spHandle) spout::spSendTexture(spHandle,ogl::oglGetRenderImage(),GL_TEXTURE_2D,w,h,true,ogl::oglGetFramebuffer());
    }
}
bool insCanSendFrames() { return loadSuccessful; }
}
