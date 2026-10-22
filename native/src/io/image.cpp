// Direct port of dlang_archive/source/nijiexpose/io/image.d.
#include "image.hpp"
#include "nijilive/texture_utils.hpp"
#include <algorithm>
namespace nijiexpose {
std::shared_ptr<nicxlive::core::Puppet> neLoadModelFromImage(const std::string& filename) {
    using namespace nicxlive::core;
    ShallowTexture tex(filename);
    inTexPremultiply(tex.data,tex.channels);
    int minX=tex.width+1,minY=tex.height+1,maxX=-1,maxY=-1;
    for(int y=0;y<tex.height;++y) {
        for(int x=0;x<tex.width;++x) {
            if(tex.data.at((y*tex.width+x)*tex.channels+tex.channels-1)!=0) {
                minX=std::min(minX,x); minY=std::min(minY,y);
                maxX=std::max(maxX,x); maxY=std::max(maxY,y);
            }
        }
    }
    std::vector<uint8_t> data(static_cast<size_t>((maxX-minX+1)*(maxY-minY+1)*tex.channels));
    for(int y=minY;y<=maxY;++y) {
        // The original excludes maxX. Retain this rather than fixing the crop.
        for(int x=minX;x<maxX;++x) {
            for(int c=0;c<tex.channels;++c)
                data.at(((y-minY)*(maxX-minX+1)+(x-minX))*tex.channels+c)=tex.data.at((y*tex.width+x)*tex.channels+c);
        }
    }
    ShallowTexture tex2(data,maxX-minX+1,maxY-minY+1,tex.channels);
    auto part=nodes::Part::createSimple(std::make_shared<Texture>(tex2),filename);
    return std::make_shared<Puppet>(part);
}
}
