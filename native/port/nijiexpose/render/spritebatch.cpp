// Port of source/nijiexpose/render/spritebatch.d.
#include "spritebatch.hpp"
#include "spritebatch_assets.hpp"
#include <core/runtime_state.hpp>
#include <algorithm>
#include <cmath>
namespace nijiexpose {
thread_local std::shared_ptr<SpriteBatch> AppBatch;
namespace {
thread_local ogl::Shader spriteBatchShader;
vec2 transformVerts(vec2 position,mat4 matrix) {
    const auto v=matrix.transformPoint({position.x,position.y,0});
    return {v.x,v.y};
}
}
void SpriteBatch::addVertexData(vec2 position,vec2 uvs,vec4 color) {
    const float values[]{position.x,position.y,uvs.x,uvs.y,color.x,color.y,color.z,color.w};
    std::copy(std::begin(values),std::end(values),data.begin()+dataOffset);
    dataOffset+=DataLength;
}
void SpriteBatch::updateVP() {
    int w,h;
    nicxlive::core::inGetViewport(w,h);
    int largest=std::max(w,h);
    viewProjection=mat4::orthographic(0,float(w),float(h),0,0,float(largest))*mat4::translation(0,0,-10);
}
SpriteBatch::SpriteBatch() : data(DataSize*EntryCount,std::numeric_limits<float>::quiet_NaN()) {
    ogl::glGenVertexArrays(1,&vao);
    ogl::glBindVertexArray(vao);
    ogl::glGenBuffers(1,&buffer);
    ogl::glBindBuffer(GL_ARRAY_BUFFER,buffer);
    ogl::glBufferData(GL_ARRAY_BUFFER,sizeof(float)*data.size(),data.data(),GL_DYNAMIC_DRAW);
    spriteBatchShader=ogl::Shader(SpriteBatchShaderSource);
    vp=spriteBatchShader.getUniformLocation("vp");
    updateVP();
}
void SpriteBatch::draw(const std::shared_ptr<nicxlive::core::Texture>& texture,SpriteRect position,SpriteRect cutout,vec2 origin,float rotation,SpriteFlip flip,vec4 color) {
    if(dataOffset==DataSize*EntryCount) flush();
    if(texture!=currentTexture) flush();
    currentTexture=texture;
    mat4 transform=mat4::translation(-origin.x,-origin.y,0)*mat4::translation(position.x,position.y,0)*
        mat4::translation(origin.x,origin.y,0)*mat4::zRotation(rotation)*mat4::translation(-origin.x,-origin.y,0)*
        mat4::scale({position.width,position.height,0});
    if(!std::isfinite(cutout.x)||!std::isfinite(cutout.y)||!std::isfinite(cutout.width)||!std::isfinite(cutout.height))
        cutout={0,0,float(texture->width()),float(texture->height())};
    constexpr double cutoffOffset=.05,cutoffAmount=cutoffOffset*2;
    bool horizontal=(static_cast<int>(flip)&static_cast<int>(SpriteFlip::Horizontal))>0;
    bool vertical=(static_cast<int>(flip)&static_cast<int>(SpriteFlip::Vertical))>0;
    vec4 uvArea{
        float(horizontal ? (cutout.x+cutout.width)-cutoffAmount : cutout.x+cutoffOffset),
        float(vertical ? (cutout.y+cutout.height)-cutoffAmount : cutout.y+cutoffOffset),
        float(horizontal ? cutout.x+cutoffOffset : (cutout.x+cutout.width)-cutoffAmount),
        float(vertical ? cutout.y+cutoffOffset : (cutout.y+cutout.height)-cutoffAmount)};
    addVertexData(transformVerts({0,1},transform),{uvArea.x,uvArea.w},color);
    addVertexData(transformVerts({1,0},transform),{uvArea.z,uvArea.y},color);
    addVertexData(transformVerts({0,0},transform),{uvArea.x,uvArea.y},color);
    addVertexData(transformVerts({0,1},transform),{uvArea.x,uvArea.w},color);
    addVertexData(transformVerts({1,1},transform),{uvArea.z,uvArea.w},color);
    addVertexData(transformVerts({1,0},transform),{uvArea.z,uvArea.y},color);
    tris+=2;
}
void SpriteBatch::flush() {
    updateVP();
    if(!currentTexture) return;
    ogl::glBindVertexArray(vao);
    ogl::glBindBuffer(GL_ARRAY_BUFFER,buffer);
    ogl::glBufferSubData(GL_ARRAY_BUFFER,0,dataOffset*sizeof(float),data.data());
    ogl::oglBindTextureHandle(currentTexture->backendId(),0);
    spriteBatchShader.use();
    spriteBatchShader.setUniform(vp,viewProjection);
    ogl::glEnableVertexAttribArray(0);
    ogl::glVertexAttribPointer(0,VecSize,GL_FLOAT,GL_FALSE,DataLength*sizeof(float),nullptr);
    ogl::glEnableVertexAttribArray(1);
    ogl::glVertexAttribPointer(1,UVSize,GL_FLOAT,GL_FALSE,DataLength*sizeof(float),reinterpret_cast<void*>(VecSize*sizeof(float)));
    ogl::glEnableVertexAttribArray(2);
    ogl::glVertexAttribPointer(2,ColorSize,GL_FLOAT,GL_FALSE,DataLength*sizeof(float),reinterpret_cast<void*>((VecSize+UVSize)*sizeof(float)));
    ogl::glDrawArrays(GL_TRIANGLES,0,static_cast<int>(tris*3));
    ogl::glDisableVertexAttribArray(0);
    ogl::glDisableVertexAttribArray(1);
    ogl::glDisableVertexAttribArray(2);
    currentTexture.reset(); dataOffset=0; tris=0;
}
}
