#include "io/package.hpp"
#include <SDL3/SDL.h>
#include <thread>
#include <string>
#include <iostream>
#include <stdexcept>

static int mode;
static SDL_Window* parent;
static std::thread worker;
static void check(bool ok) { if(!ok) throw std::runtime_error("dialog assertion failed"); }
extern "C" void __wrap_SDL_ShowFileDialogWithProperties(SDL_FileDialogType type,
    SDL_DialogFileCallback callback, void* data, SDL_PropertiesID props) {
    auto* filters=static_cast<const SDL_DialogFileFilter*>(SDL_GetPointerProperty(props,SDL_PROP_FILE_DIALOG_FILTERS_POINTER,nullptr));
    check(type==SDL_FILEDIALOG_OPENFILE);
    check(SDL_GetPointerProperty(props,SDL_PROP_FILE_DIALOG_WINDOW_POINTER,nullptr)==parent);
    check(SDL_GetNumberProperty(props,SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER,0)==2);
    check(!SDL_GetBooleanProperty(props,SDL_PROP_FILE_DIALOG_MANY_BOOLEAN,true));
    check(std::string(SDL_GetStringProperty(props,SDL_PROP_FILE_DIALOG_TITLE_STRING,""))=="Open...");
    auto complete=[=] {
        check(std::string(filters[0].pattern)=="inp;inx");
        check(std::string(filters[0].name)=="Models");
        check(std::string(filters[1].pattern)=="*");
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
    parent=SDL_CreateWindow("Dialog test",32,32,SDL_WINDOW_HIDDEN);
    check(parent!=nullptr);
    for(mode=0;mode<5;++mode) {
        SDL_Event event{};event.type=SDL_EVENT_USER;SDL_PushEvent(&event);
        const nijiexpose::io::TFD_Filter filters[]={{{"*.inp","*.inx"},"Models"}};
        auto path=nijiexpose::io::insShowOpenDialog(filters,"Open...",parent);
        if(worker.joinable()) worker.join();
        check(path==((mode==0||mode==4)?"/tmp/model.inp":""));
        check(SDL_HasEvent(SDL_EVENT_USER));
        if(mode==3) check(SDL_HasEvent(SDL_EVENT_QUIT));
        SDL_FlushEvents(SDL_EVENT_FIRST,SDL_EVENT_LAST);
    }
    SDL_DestroyWindow(parent);
    SDL_Quit();
    std::cout<<"select, cancel, error, late callback after quit, synchronous callback: passed\n";
}
