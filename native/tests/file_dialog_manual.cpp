#include "nijiexpose/io/package.hpp"
#include <SDL3/SDL.h>
#include <iostream>
int main(int argc,char** argv) {
    if(!SDL_Init(SDL_INIT_VIDEO)) return 2;
    auto* parent=SDL_CreateWindow("Dialog verification parent",640,480,0);
    if(!parent) return 2;
    const nijiexpose::io::TFD_Filter filters[]={{{"*.inp","*.inx"},"Models"}};
    auto path=nijiexpose::io::insShowOpenDialog(filters,"Nijiexpose dialog verification",parent);
    std::cout<<path<<std::endl;
    SDL_DestroyWindow(parent);
    SDL_Quit();
    return path==(argc==2?argv[1]:"")?0:1;
}
