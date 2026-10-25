#pragma once
namespace nijiexpose {
void insInitFrameSending();
void insCleanupFrameSending();
void insSendFrame();
bool insCanSendFrames();
#ifdef _WIN32
const char* insFrameSenderName();
#endif
}
