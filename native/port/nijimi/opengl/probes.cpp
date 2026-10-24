// nijimi CPU texture records, sampleTextureAlpha and final backbuffer alpha probe.
#include "probes.hpp"
#include "api.hpp"
#include <unordered_map>
#include <vector>
namespace nijiexpose::ogl {
struct CpuTextureRecord {int width{},height{},channels{};std::vector<uint8_t> pixels;};
static std::unordered_map<uint32_t,CpuTextureRecord> records;
static bool pending=false,valid=false;
static int localX{},localY{},windowW{},windowH{};
static uint8_t sample=255;
void oglRecordCpuTexture(uint32_t handle,int w,int h,int channels,std::span<const uint8_t> pixels) {
    records[handle]={w,h,channels,{pixels.begin(),pixels.end()}};
}
void oglReleaseCpuTexture(uint32_t handle) {records.erase(handle);}
bool oglSampleTextureAlpha(uint32_t handle,float u,float v,uint8_t& alpha) {
    alpha=0;
    if(!handle)return false;
    auto it=records.find(handle);
    if(it==records.end())return false;
    const auto& rec=it->second;
    if(rec.width<=0||rec.height<=0||rec.channels<=0)return false;
    const auto expected=size_t(rec.width)*size_t(rec.height)*size_t(rec.channels);
    if(rec.pixels.size()<expected)return false;
    float uu=u,vv=v;
    if(uu<0)uu=0;if(uu>1)uu=1;if(vv<0)vv=0;if(vv>1)vv=1;
    auto xi=size_t(uu*float(rec.width-1)),yi=size_t(vv*float(rec.height-1));
    auto idx=(yi*size_t(rec.width)+xi)*size_t(rec.channels);
    if(idx>=rec.pixels.size())return false;
    if(rec.channels>=4&&idx+3<rec.pixels.size())alpha=rec.pixels[idx+3];
    else if(rec.channels==2&&idx+1<rec.pixels.size())alpha=rec.pixels[idx+1];
    else alpha=255;
    return true;
}
void oglSetFinalAlphaProbe(int x,int y,int w,int h) {
    if(w<=0||h<=0){pending=false;valid=false;return;}
    localX=x;localY=y;windowW=w;windowH=h;pending=true;
}
bool oglConsumeFinalAlphaSample(uint8_t& alpha) {
    alpha=255;if(!valid)return false;alpha=sample;valid=false;return true;
}
void oglCaptureFinalAlphaProbe(int w,int h) {
    if(pending&&w>0&&h>0&&windowW>0&&windowH>0) {
        int px=int((float(localX)/float(windowW))*float(w));
        int py=int((float(localY)/float(windowH))*float(h));
        if(px<0)px=0;if(py<0)py=0;if(px>=w)px=w-1;if(py>=h)py=h-1;
        uint8_t rgba[4]={0,0,0,255};
        glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
        glReadPixels(px,h-1-py,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
        sample=rgba[3];valid=true;
    } else valid=false;
    pending=false;
}
}
