#include "spout.hpp"
namespace nijiexpose::spout {
spGetSpoutType spGetSpout=nullptr;
spSetSenderNameType spSetSenderName=nullptr;
spSetSenderFormatType spSetSenderFormat=nullptr;
spReleaseSenderType spReleaseSender=nullptr;
spSendFboType spSendFbo=nullptr;
spSendTextureType spSendTexture=nullptr;
spSendImageType spSendImage=nullptr;
spGetNameType spGetName=nullptr;
spGetWidthType spGetWidth=nullptr;
spGetHeightType spGetHeight=nullptr;
spGetFpsType spGetFps=nullptr;
spGetFrameType spGetFrame=nullptr;
spGetHandleType spGetHandle=nullptr;
spGetCPUType spGetCPU=nullptr;
spGetGLDXType spGetGLDX=nullptr;
spGetSenderCountType spGetSenderCount=nullptr;
spGetSenderType spGetSender=nullptr;
spFindSenderNameType spFindSenderName=nullptr;
spGetSenderInfoType spGetSenderInfo=nullptr;
spGetActiveSenderType spGetActiveSender=nullptr;
spSetActiveSenderType spSetActiveSender=nullptr;
namespace { thread_local HMODULE lib=nullptr; thread_local Spout2Support loadedVersion=Spout2Support::noLibrary; }
void unloadSpout2() { if(lib) { FreeLibrary(lib); lib=nullptr; } }
Spout2Support loadedSpout2Version() { return loadedVersion; }
bool isSpout2Loaded() { return lib!=nullptr; }
Spout2Support loadSpout2(const char* libName) {
    lib=LoadLibraryA(libName);
    if(!lib) return Spout2Support::noLibrary;
    loadedVersion=Spout2Support::badLibrary;
    spGetSpout=reinterpret_cast<spGetSpoutType>(GetProcAddress(lib,"GetSpout"));
    spSetSenderName=reinterpret_cast<spSetSenderNameType>(GetProcAddress(lib,"spSetSenderName"));
    spSetSenderFormat=reinterpret_cast<spSetSenderFormatType>(GetProcAddress(lib,"spSetSenderFormat"));
    spReleaseSender=reinterpret_cast<spReleaseSenderType>(GetProcAddress(lib,"spReleaseSender"));
    spSendFbo=reinterpret_cast<spSendFboType>(GetProcAddress(lib,"spSendFbo"));
    spSendTexture=reinterpret_cast<spSendTextureType>(GetProcAddress(lib,"spSendTexture"));
    spSendImage=reinterpret_cast<spSendImageType>(GetProcAddress(lib,"spSendImage"));
    spGetName=reinterpret_cast<spGetNameType>(GetProcAddress(lib,"spGetName"));
    spGetWidth=reinterpret_cast<spGetWidthType>(GetProcAddress(lib,"spGetWidth"));
    spGetHeight=reinterpret_cast<spGetHeightType>(GetProcAddress(lib,"spGetHeight"));
    spGetFps=reinterpret_cast<spGetFpsType>(GetProcAddress(lib,"spGetFps"));
    spGetFrame=reinterpret_cast<spGetFrameType>(GetProcAddress(lib,"spGetFrame"));
    spGetHandle=reinterpret_cast<spGetHandleType>(GetProcAddress(lib,"spGetHandle"));
    spGetCPU=reinterpret_cast<spGetCPUType>(GetProcAddress(lib,"spGetCPU"));
    spGetGLDX=reinterpret_cast<spGetGLDXType>(GetProcAddress(lib,"spGetGLDX"));
    spGetSenderCount=reinterpret_cast<spGetSenderCountType>(GetProcAddress(lib,"spGetSenderCount"));
    spGetSender=reinterpret_cast<spGetSenderType>(GetProcAddress(lib,"spGetSender"));
    spFindSenderName=reinterpret_cast<spFindSenderNameType>(GetProcAddress(lib,"spFindSenderName"));
    spGetSenderInfo=reinterpret_cast<spGetSenderInfoType>(GetProcAddress(lib,"spGetSenderInfo"));
    spGetActiveSender=reinterpret_cast<spGetActiveSenderType>(GetProcAddress(lib,"spGetActiveSender"));
    spSetActiveSender=reinterpret_cast<spSetActiveSenderType>(GetProcAddress(lib,"spSetActiveSender"));
    return Spout2Support::spout2;
}
}
