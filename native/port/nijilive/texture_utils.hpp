#pragma once
#include <span>
#include <cstdint>
namespace nijiexpose {
inline void inTexPremultiply(std::span<uint8_t> data,int channels=4) {
    if(channels<4) return;
    for(size_t i=0;i<data.size()/channels;++i) {
        size_t offset=i*channels;
        data[offset+0]=static_cast<uint8_t>((int(data[offset+0])*int(data[offset+3]))/255);
        data[offset+1]=static_cast<uint8_t>((int(data[offset+1])*int(data[offset+3]))/255);
        data[offset+2]=static_cast<uint8_t>((int(data[offset+2])*int(data[offset+3]))/255);
    }
}
inline void inTexUnPremuliply(std::span<uint8_t> data) {
    for(size_t i=0;i<data.size()/4;++i) {
        if(data[i*4+3]==0) continue;
        data[i*4+0]=static_cast<uint8_t>(int(data[i*4+0])*255/int(data[i*4+3]));
        data[i*4+1]=static_cast<uint8_t>(int(data[i*4+1])*255/int(data[i*4+3]));
        data[i*4+2]=static_cast<uint8_t>(int(data[i*4+2])*255/int(data[i*4+3]));
    }
}
}
