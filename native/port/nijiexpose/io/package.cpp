// tinyfiledialogs 0.10.1: Windows GUI / single-select path called by nijiexpose.
#include "package.hpp"
#include "dcompat/string.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <objbase.h>
#include <array>
#include <cstdio>
namespace nijiexpose::io {
std::wstring buildFilterString(std::span<const TFD_Filter> filters){
    std::string text;
    for(auto& filter:filters){std::vector<std::string> patterns;for(auto& p:filter.patterns){auto v=dcompat::fromStringz(p);if(!v.empty()){patterns.emplace_back(v);if(patterns.size()==8)break;}}
        if(patterns.empty())continue;size_t mime=0;for(auto& p:patterns){if(p.find('/')!=std::string::npos)++mime;else break;}if(mime>0)continue;
        std::string joined;for(auto& p:patterns){if(!joined.empty())joined+=';';joined+=p;}auto description=dcompat::fromStringz(filter.description);text+=description.empty()?joined:std::string(description);text+='\n';text+=joined;text+='\n';
    }text+="All Files\n*.*\n";auto wide=dcompat::toUTF16z(text);for(auto& c:wide)if(c=='\n')c=0;return wide;
}
std::string insShowOpenDialog(std::span<const TFD_Filter> filters,const std::string& title,const std::string& parentWindow){
    (void)parentWindow;
    std::array<wchar_t,1024> buffer{};auto wideTitle=dcompat::toUTF16z(title),filter=buildFilterString(filters);HRESULT initialized=CoInitializeEx(nullptr,0);
    OPENFILENAMEW ofn{};ofn.lStructSize=sizeof(OPENFILENAMEW);ofn.hwndOwner=GetForegroundWindow();ofn.lpstrFilter=filter.c_str();ofn.nFilterIndex=1;ofn.lpstrFile=buffer.data();ofn.nMaxFile=1024;ofn.nMaxFileTitle=512;ofn.lpstrTitle=wideTitle.empty()||wideTitle[0]==0?nullptr:wideTitle.c_str();ofn.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    bool selected=GetOpenFileNameW(&ofn)!=0;if(initialized==S_OK||initialized==S_FALSE)CoUninitialize();if(!selected)return {};
    auto path=dcompat::toUTF8(std::wstring_view(buffer.data()));DWORD attrs=GetFileAttributesW(buffer.data());if((attrs&FILE_ATTRIBUTE_DEVICE)||(attrs&FILE_ATTRIBUTE_DIRECTORY))return {};return path;
}
}
