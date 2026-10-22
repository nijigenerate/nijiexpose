#include "application.hpp"
#include "log.hpp"
#include "control.hpp"
#include "dcompat/string.hpp"
#include <imgui.h>
#include <cstdio>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif
static int runApplication(int argc,char** argv){
    std::vector<std::string> args;
    std::string controlDirectory;
    for(int i=1;i<argc;++i){
        auto arg=std::string(argv[i]);
        if(arg=="--control-dir"){
            if(i+1==argc){std::fputs("--control-dir requires a directory\n",stderr);return 1;}
            controlDirectory=std::string(argv[++i]);continue;
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
        if(!controlDirectory.empty())nijiexpose::control::start(nijiexpose::dcompat::pathFromUTF8(controlDirectory));
        while(window->isAlive())window->update();
        nijiexpose::shutdownApplication();
        return 0;
    }catch(const std::exception& ex){nijiexpose::insLogErr("%s",ex.what());return 1;}
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int){
    int argc=0;
    auto* wide=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(!wide)return 1;
    struct Arguments { wchar_t** value; ~Arguments(){LocalFree(value);} } arguments{wide};
    std::vector<std::string> owned;
    std::vector<char*> argv;
    owned.reserve(static_cast<std::size_t>(argc)); argv.reserve(static_cast<std::size_t>(argc));
    for(int i=0;i<argc;++i)owned.push_back(nijiexpose::dcompat::toUTF8(wide[i]));
    for(auto& value:owned)argv.push_back(value.data());
    return runApplication(argc,argv.data());
}
#else
int main(int argc,char** argv){ return runApplication(argc,argv); }
#endif
