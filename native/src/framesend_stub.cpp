#include "framesend.hpp"
#include "log.hpp"
#include "i18n/tr.hpp"
namespace nijiexpose {
void insInitFrameSending() {
    insLogInfo(i18n::_("Frame sharing is unavailable on this platform."));
}
void insCleanupFrameSending() {}
void insSendFrame() {}
bool insCanSendFrames() { return false; }
}
