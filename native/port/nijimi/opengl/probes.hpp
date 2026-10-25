#pragma once
#include <cstdint>
#include <span>
namespace nijiexpose::ogl {
void oglRecordCpuTexture(uint32_t handle,int width,int height,int channels,std::span<const uint8_t> pixels);
void oglReleaseCpuTexture(uint32_t handle);
bool oglSampleTextureAlpha(uint32_t handle,float u,float v,uint8_t& alpha);
void oglSetFinalAlphaProbe(int localX,int localY,int windowW,int windowH);
bool oglConsumeFinalAlphaSample(uint8_t& alpha);
void oglCaptureFinalAlphaProbe(int drawableW,int drawableH);
}
