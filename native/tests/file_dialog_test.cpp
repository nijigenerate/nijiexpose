#include "nijiexpose/io/package.hpp"
#include <SDL3/SDL.h>
#include <thread>
#include <string>
#include <iostream>
#include <stdexcept>

static int mode;
static std::thread worker;
static void check(bool ok) { if(!ok) throw std::runtime_error("dialog assertion failed"); }
extern "C" void __wrap_SDL_ShowOpenFileDialog(SDL_DialogFileCallback callback, void* data,
    SDL_Window*, const SDL_DialogFileFilter* filters, int count, const char*, bool many) {
    check(count==1 && !many);
    auto complete=[=] {
        check(std::string(filters[0].pattern)=="inp;inx");
        check(std::string(filters[0].name)=="Models");
        const char* selected[]={"/tmp/model.inp",nullptr};
        const char* canceled[]={nullptr};
        callback(data,mode==2?nullptr:mode==1?canceled:selected,0);
    };
    if(mode==4) { complete(); return; }
    worker=std::thread([=] {
        if(mode==3) { SDL_Event e{};e.type=SDL_EVENT_QUIT;SDL_PushEvent(&e); }
        SDL_Delay(100);
        complete();
    });
}
int main() {
    check(SDL_Init(SDL_INIT_VIDEO));
    for(mode=0;mode<5;++mode) {
        SDL_Event event{};event.type=SDL_EVENT_USER;SDL_PushEvent(&event);
        const nijiexpose::io::TFD_Filter filters[]={{{"*.inp","*.inx"},"Models"}};
        auto path=nijiexpose::io::insShowOpenDialog(filters);
        if(worker.joinable()) worker.join();
        check(path==((mode==0||mode==4)?"/tmp/model.inp":""));
        check(SDL_HasEvent(SDL_EVENT_USER));
        if(mode==3) check(SDL_HasEvent(SDL_EVENT_QUIT));
        SDL_FlushEvents(SDL_EVENT_FIRST,SDL_EVENT_LAST);
    }
    SDL_Quit();
    std::cout<<"select, cancel, error, late callback after quit, synchronous callback: passed\n";
}
