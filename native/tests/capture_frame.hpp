#pragma once
#include "nijilive/opengl/api.hpp"
#include "presented_frame.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
inline void savePngRGB(const std::string& path,int width,int height,const std::vector<unsigned char>& pixels){
    using Bytes=std::vector<unsigned char>;Bytes png{137,80,78,71,13,10,26,10};
    auto be=[](Bytes& out,uint32_t v){for(int n=24;n>=0;n-=8)out.push_back(static_cast<unsigned char>(v>>n));};
    auto chunk=[&](const char* name,const Bytes& data){be(png,static_cast<uint32_t>(data.size()));size_t start=png.size();png.insert(png.end(),name,name+4);png.insert(png.end(),data.begin(),data.end());uint32_t crc=0xffffffff;for(size_t i=start;i<png.size();++i){crc^=png[i];for(int j=0;j<8;++j)crc=(crc>>1)^(0xedb88320u&uint32_t(-int(crc&1)));}be(png,crc^0xffffffff);};
    Bytes header;be(header,width);be(header,height);header.insert(header.end(),{8,2,0,0,0});chunk("IHDR",header);
    Bytes raw;for(int y=0;y<height;++y){raw.push_back(0);raw.insert(raw.end(),pixels.begin()+static_cast<size_t>(y)*width*3,pixels.begin()+static_cast<size_t>(y+1)*width*3);}
    Bytes z{0x78,0x01};for(size_t pos=0;pos<raw.size();){uint16_t n=static_cast<uint16_t>(std::min<size_t>(65535,raw.size()-pos));z.push_back(pos+n==raw.size()?1:0);z.push_back(n&255);z.push_back(n>>8);uint16_t inverse=~n;z.push_back(inverse&255);z.push_back(inverse>>8);z.insert(z.end(),raw.begin()+pos,raw.begin()+pos+n);pos+=n;}
    uint32_t a=1,b=0;for(auto v:raw){a=(a+v)%65521;b=(b+a)%65521;}be(z,(b<<16)|a);chunk("IDAT",z);chunk("IEND",{});
    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(png.data()),png.size());if(!out)throw std::runtime_error("Cannot save captured framebuffer");
}
inline void captureFrontBuffer(const std::string& path,int width,int height){
    nijiexpose::ogl::loadApi();
    nijiexpose::ogl::glBindFramebuffer(GL_FRAMEBUFFER,0);nijiexpose::ogl::glReadBuffer(presentedReadBuffer());nijiexpose::ogl::glPixelStorei(GL_PACK_ALIGNMENT,1);
    std::vector<unsigned char> pixels(static_cast<size_t>(width)*height*3);readPresentedPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    for(int y=0;y<height/2;++y)for(int x=0;x<width*3;++x)std::swap(pixels[static_cast<size_t>(y)*width*3+x],pixels[static_cast<size_t>(height-1-y)*width*3+x]);
    savePngRGB(path,width,height,pixels);
}
