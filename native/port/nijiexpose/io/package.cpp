#include "package.hpp"
#include "dcompat/string.hpp"
#include <SDL3/SDL.h>
#include <atomic>
#include <memory>
namespace nijiexpose::io {
std::string insShowOpenDialog(std::span<const TFD_Filter> filters,const std::string& title,SDL_Window* parentWindow){
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
            if(value=="*.*" || value=="*") pattern+="*";
            else if(value.starts_with("*.")) pattern+=value.substr(2);
            else if(value.starts_with('.')) pattern+=value.substr(1);
            else pattern+=value;
        }
        if(pattern.empty()) continue;
        descriptions.emplace_back(dcompat::fromStringz(filter.description));
        patterns.push_back(std::move(pattern));
    }
    descriptions.emplace_back("All Files");
    patterns.emplace_back("*");
    for(std::size_t i=0;i<patterns.size();++i)
        nativeFilters.push_back({descriptions[i].empty()?"Files":descriptions[i].c_str(),patterns[i].c_str()});
    auto callback=[](void* userdata,const char* const* files,int) {
        std::unique_ptr<std::shared_ptr<Result>> owner(static_cast<std::shared_ptr<Result>*>(userdata));
        auto& result=**owner;
        if(files&&files[0]) result.path=files[0];
        result.done.store(true,std::memory_order_release);
    };
    const auto properties=SDL_CreateProperties();
    if(!properties) return {};
    const bool configured=
        SDL_SetPointerProperty(properties,SDL_PROP_FILE_DIALOG_FILTERS_POINTER,nativeFilters.data()) &&
        SDL_SetNumberProperty(properties,SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER,static_cast<Sint64>(nativeFilters.size())) &&
        SDL_SetPointerProperty(properties,SDL_PROP_FILE_DIALOG_WINDOW_POINTER,parentWindow?parentWindow:SDL_GetKeyboardFocus()) &&
        SDL_SetStringProperty(properties,SDL_PROP_FILE_DIALOG_TITLE_STRING,title.c_str()) &&
        SDL_SetBooleanProperty(properties,SDL_PROP_FILE_DIALOG_MANY_BOOLEAN,false);
    if(!configured) { SDL_DestroyProperties(properties); return {}; }
    SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFILE,callback,new std::shared_ptr<Result>(result),properties);
    SDL_DestroyProperties(properties);
    while(!result->done.load(std::memory_order_acquire)) {
        // Pump platform events without consuming input intended for the app.
        SDL_PumpEvents();
        if(SDL_HasEvent(SDL_EVENT_QUIT)) return {};
        SDL_Delay(10);
    }
    return result->path;
}
}
