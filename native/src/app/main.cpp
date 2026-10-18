#include "nijiexpose/application.hpp"
#include "nijiexpose/log.hpp"
#include "nijiexpose/control.hpp"
#include "dcompat/string.hpp"
#include <imgui.h>
#include <cstdio>
int wmain(int argc,wchar_t** argv){
    std::vector<std::string> args;
    std::string controlDirectory;
    for(int i=1;i<argc;++i){
        auto arg=nijiexpose::dcompat::toUTF8(argv[i]);
        if(arg=="--control-dir"){
            if(i+1==argc){std::fputs("--control-dir requires a directory\n",stderr);return 1;}
            controlDirectory=nijiexpose::dcompat::toUTF8(argv[++i]);continue;
        }
        if(arg=="--version"){
            std::puts("nijiexpose-native " NIJIEXPOSE_NATIVE_VERSION " / nicxlive / SDL3 / ImGui " IMGUI_VERSION " / Boost.JSON / GNU gettext");return 0;
        }
        if(arg=="--help"){std::puts("Usage: nijiexpose-native [--control-dir DIRECTORY] [model.inp | model.inx | image ...]");return 0;}
        if(arg.starts_with("--")){std::fprintf(stderr,"Unknown option: %s\n",arg.c_str());return 1;}
        args.push_back(std::move(arg));
    }
    nijiexpose::insInitLogging();
    nijiexpose::insLogInfo("nijiexpose %s, args=%s",NIJIEXPOSE_NATIVE_VERSION,args);
    try{
        auto window=nijiexpose::initializeApplication(args);
        if(!controlDirectory.empty())nijiexpose::control::start(std::filesystem::u8path(controlDirectory));
        while(window->isAlive())window->update();
        nijiexpose::shutdownApplication();
        return 0;
    }catch(const std::exception& ex){nijiexpose::insLogErr("%s",ex.what());return 1;}
}
