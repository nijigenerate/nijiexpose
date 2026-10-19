#include "nijiexpose/io/package.hpp"
#include <SDL3/SDL.h>
#include <iostream>
int main(int argc,char** argv) {
    if(!SDL_Init(SDL_INIT_VIDEO)) return 2;
    const nijiexpose::io::TFD_Filter filters[]={{{"*.inp","*.inx"},"Models"}};
    auto path=nijiexpose::io::insShowOpenDialog(filters);
    std::cout<<path<<std::endl;
    SDL_Quit();
    return argc==2 && path==argv[1]?0:1;
}
