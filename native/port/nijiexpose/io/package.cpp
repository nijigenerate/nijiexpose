// tinyfiledialogs 0.10.1: Windows GUI / single-select path called by nijiexpose.
#include "package.hpp"
#include "dcompat/string.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <objbase.h>
#else
#include <SDL3/SDL.h>
#include <atomic>
#include <memory>
#endif
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
#ifdef _WIN32
    std::array<wchar_t,1024> buffer{};auto wideTitle=dcompat::toUTF16z(title),filter=buildFilterString(filters);HRESULT initialized=CoInitializeEx(nullptr,0);
    OPENFILENAMEW ofn{};ofn.lStructSize=sizeof(OPENFILENAMEW);ofn.hwndOwner=GetForegroundWindow();ofn.lpstrFilter=filter.c_str();ofn.nFilterIndex=1;ofn.lpstrFile=buffer.data();ofn.nMaxFile=1024;ofn.nMaxFileTitle=512;ofn.lpstrTitle=wideTitle.empty()||wideTitle[0]==0?nullptr:wideTitle.c_str();ofn.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    bool selected=GetOpenFileNameW(&ofn)!=0;if(initialized==S_OK||initialized==S_FALSE)CoUninitialize();if(!selected)return {};
    auto path=dcompat::toUTF8(std::wstring_view(buffer.data()));DWORD attrs=GetFileAttributesW(buffer.data());if((attrs&FILE_ATTRIBUTE_DEVICE)||(attrs&FILE_ATTRIBUTE_DIRECTORY))return {};return path;
#else
    // The callback may run on another thread, including after a quit request.
    struct Result {
        std::atomic<bool> done{false};
        std::string path;
        std::vector<std::string> descriptions,patterns;
        std::vector<SDL_DialogFileFilter> filters;
    };
    auto result=std::make_shared<Result>();
    auto& descriptions=result->descriptions;
    auto& patterns=result->patterns;
    auto& nativeFilters=result->filters;
    for(const auto& filter:filters) {
        std::string pattern;
        for(const auto& item:filter.patterns) {
            const auto value=dcompat::fromStringz(item);
            if(value.empty()||value.find('/')!=std::string_view::npos) continue;
            if(!pattern.empty()) pattern.push_back(';');
            if(value.starts_with("*.")) pattern+=value.substr(2);
            else if(value.starts_with('.')) pattern+=value.substr(1);
            else pattern+=value;
        }
        if(pattern.empty()) continue;
        descriptions.emplace_back(dcompat::fromStringz(filter.description));
        patterns.push_back(std::move(pattern));
    }
    for(std::size_t i=0;i<patterns.size();++i)
        nativeFilters.push_back({descriptions[i].empty()?"Files":descriptions[i].c_str(),patterns[i].c_str()});
    auto callback=[](void* userdata,const char* const* files,int) {
        std::unique_ptr<std::shared_ptr<Result>> owner(static_cast<std::shared_ptr<Result>*>(userdata));
        auto& result=**owner;
        if(files&&files[0]) result.path=files[0];
        result.done.store(true,std::memory_order_release);
    };
    SDL_ShowOpenFileDialog(callback,new std::shared_ptr<Result>(result),nullptr,nativeFilters.data(),static_cast<int>(nativeFilters.size()),nullptr,false);
    while(!result->done.load(std::memory_order_acquire)) {
        // Pump platform events without consuming input intended for the app.
        SDL_PumpEvents();
        if(SDL_HasEvent(SDL_EVENT_QUIT)) return {};
        SDL_Delay(10);
    }
    return result->path;
#endif
}
}
