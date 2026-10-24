#pragma once
#include <string>
namespace nijiui {
enum class DialogLevel : size_t { Info=0,Warning=1,Error=2 };
enum class DialogButtons { NONE=0,OK=1,Cancel=2,Yes=4,No=8 };
constexpr DialogButtons operator|(DialogButtons a,DialogButtons b) { return static_cast<DialogButtons>(int(a)|int(b)); }
void uiImInitDialogs();
void uiImRenderDialogs();
void uiImCleanupDialogs();
void uiImDialog(const char* title,std::string body,DialogLevel level=DialogLevel::Error,DialogButtons buttons=DialogButtons::OK,void* userData=nullptr);
void uiImDialog(const char* tag,const char* title,std::string body,DialogLevel level=DialogLevel::Error,DialogButtons buttons=DialogButtons::OK,void* userData=nullptr);
DialogButtons uiImDialogButtonSelected(const char* tag);
void* uiImDialogButtonUserData(const char* tag);
}
