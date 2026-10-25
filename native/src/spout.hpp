// ABI port of bindbc-spout2 0.1.1. See native/licenses/bindbc-spout2.txt.
#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
namespace nijiexpose::spout {
struct SPOUTLIBRARY;
using SPOUTHANDLE=SPOUTLIBRARY*;
enum class Spout2Support { noLibrary,badLibrary,spout2 };
Spout2Support loadSpout2(const char* libName="SpoutLibrary.dll");
void unloadSpout2();
Spout2Support loadedSpout2Version();
bool isSpout2Loaded();
using spGetSpoutType = SPOUTHANDLE (__cdecl*)();
extern spGetSpoutType spGetSpout;
using spSetSenderNameType = void (__cdecl*)(SPOUTHANDLE self, const char* senderName);
extern spSetSenderNameType spSetSenderName;
using spSetSenderFormatType = void (__cdecl*)(SPOUTHANDLE self, DWORD dwFormat);
extern spSetSenderFormatType spSetSenderFormat;
using spReleaseSenderType = void (__cdecl*)(SPOUTHANDLE self, DWORD dwMsec);
extern spReleaseSenderType spReleaseSender;
using spSendFboType = bool (__cdecl*)(SPOUTHANDLE self, uint32_t fboId, uint32_t width, uint32_t height, bool invert);
extern spSendFboType spSendFbo;
using spSendTextureType = bool (__cdecl*)(SPOUTHANDLE self, uint32_t textureId, uint32_t textureTarget, uint32_t width, uint32_t height, bool invert, uint32_t hostFBO);
extern spSendTextureType spSendTexture;
using spSendImageType = bool (__cdecl*)(SPOUTHANDLE self, const uint8_t* pixels, uint32_t width, uint32_t height, uint32_t glFormat, bool invert);
extern spSendImageType spSendImage;
using spGetNameType = const char* (__cdecl*)(SPOUTHANDLE self);
extern spGetNameType spGetName;
using spGetWidthType = uint32_t (__cdecl*)(SPOUTHANDLE self);
extern spGetWidthType spGetWidth;
using spGetHeightType = uint32_t (__cdecl*)(SPOUTHANDLE self);
extern spGetHeightType spGetHeight;
using spGetFpsType = double (__cdecl*)(SPOUTHANDLE self);
extern spGetFpsType spGetFps;
using spGetFrameType = int64_t (__cdecl*)(SPOUTHANDLE self);
extern spGetFrameType spGetFrame;
using spGetHandleType = HANDLE (__cdecl*)(SPOUTHANDLE self);
extern spGetHandleType spGetHandle;
using spGetCPUType = bool (__cdecl*)(SPOUTHANDLE self);
extern spGetCPUType spGetCPU;
using spGetGLDXType = bool (__cdecl*)(SPOUTHANDLE self);
extern spGetGLDXType spGetGLDX;
using spGetSenderCountType = int (__cdecl*)(SPOUTHANDLE self);
extern spGetSenderCountType spGetSenderCount;
using spGetSenderType = bool (__cdecl*)(SPOUTHANDLE self, int index, char* sendername, int MaxSize);
extern spGetSenderType spGetSender;
using spFindSenderNameType = bool (__cdecl*)(SPOUTHANDLE self, char* sendername);
extern spFindSenderNameType spFindSenderName;
using spGetSenderInfoType = bool (__cdecl*)(SPOUTHANDLE self, const char* sendername, uint32_t& width, uint32_t& height, HANDLE& dxShareHandle, DWORD& dwFormat);
extern spGetSenderInfoType spGetSenderInfo;
using spGetActiveSenderType = bool (__cdecl*)(SPOUTHANDLE self, char* sendername);
extern spGetActiveSenderType spGetActiveSender;
using spSetActiveSenderType = bool (__cdecl*)(SPOUTHANDLE self, char* sendername);
extern spSetActiveSenderType spSetActiveSender;
}
