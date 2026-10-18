// Adapter for the D OpenGL implementation and nicxlive's C++ renderer interface.
#include "backend.hpp"
#include "runtime.hpp"
#include "drawable.hpp"
#include "debug.hpp"
#include "part.hpp"
#include "mask.hpp"
#include "blending.hpp"
#include "draw_texture.hpp"
#include "render_queue.hpp"
#include "difference.hpp"
#include <algorithm>
#include <stdexcept>
namespace nijiexpose::ogl {
void Backend::initializeRenderer() {
    loadApi();
    oglInitRenderer();
    oglInitDrawableBackend();
    oglInitPartBackendResources();
    oglInitMaskBackend();
}
void Backend::initializeDrawableResources() { oglInitDrawableBackend(); }
void Backend::bindDrawableVao() { oglBindDrawableVao(); }
void Backend::createDrawableBuffers(uint32_t& id) { oglCreateDrawableBuffers(id); }
void Backend::uploadDrawableIndices(uint32_t id, const std::vector<uint16_t>& data) { oglUploadDrawableIndices(id,data); }
void Backend::uploadSharedVertexBuffer(const Vec2Array& data) { oglUploadSharedVertexBuffer(data); }
void Backend::uploadSharedUvBuffer(const Vec2Array& data) { oglUploadSharedUvBuffer(data); }
void Backend::uploadSharedDeformBuffer(const Vec2Array& data) { oglUploadSharedDeformBuffer(data); }
void Backend::drawDrawableElements(uint32_t id, size_t count) { oglDrawDrawableElements(id,count); }
void Backend::setDebugPointSize(float value) { oglSetDebugPointSize(value); }
void Backend::setDebugLineWidth(float value) { oglSetDebugLineWidth(value); }
void Backend::uploadDebugBuffer(const std::vector<vec3>& points, const std::vector<uint16_t>& indices) { oglUploadDebugBuffer(points,indices); }
void Backend::drawDebugPoints(const vec4& color, const mat4& matrix) { oglDrawDebugPoints(color,matrix); }
void Backend::drawDebugLines(const vec4& color, const mat4& matrix) { oglDrawDebugLines(color,matrix); }
void Backend::beginMask(bool stencil) { oglBeginMask(stencil); }
void Backend::applyMask(const MaskApplyPacket& packet) { oglExecuteMaskApplyPacket(packet); }
void Backend::beginMaskContent() { oglBeginMaskContent(); }
void Backend::endMask() { oglEndMask(); }
uint32_t Backend::createTexture(const std::vector<uint8_t>& data,int w,int h,int channels,bool stencil) {
    return createNativeTexture(data,w,h,channels,channels,stencil,true);
}
uint32_t Backend::createNativeTexture(const std::vector<uint8_t>& data,int w,int h,int inChannels,int outChannels,bool stencil,bool mipmaps) {
    GLuint id{};
    oglCreateTextureHandle(id);
    textures.emplace(id, TextureInfo{w,h,outChannels,stencil,mipmaps});
    setNativeTextureData(id,data,inChannels);
    setNativeTextureFiltering(id,nicxlive::core::Filtering::Linear,mipmaps);
    setNativeTextureWrapping(id,nicxlive::core::Wrapping::Clamp);
    setNativeTextureAnisotropy(id,oglMaxTextureAnisotropy()/2.0f);
    return id;
}
void Backend::setNativeTextureData(uint32_t id,const std::vector<uint8_t>& data,int inChannels) {
    const auto& info = textures.at(id);
    oglUploadTextureData(id,info.width,info.height,inChannels,info.channels,info.stencil,data);
    if (!info.stencil && info.mipmaps) oglGenerateTextureMipmap(id);
}
void Backend::setNativeTextureFiltering(uint32_t id,nicxlive::core::Filtering value,bool mipmaps) {
    oglApplyTextureFiltering(id,value == nicxlive::core::Filtering::Linear ? Filtering::Linear : Filtering::Nearest,mipmaps);
}
void Backend::setNativeTextureWrapping(uint32_t id,nicxlive::core::Wrapping value) {
    oglApplyTextureWrapping(id,static_cast<Wrapping>(value));
}
void Backend::setNativeTextureAnisotropy(uint32_t id,float value) {
    oglApplyTextureAnisotropy(id,std::clamp(value,1.f,oglMaxTextureAnisotropy()));
}
std::vector<uint8_t> Backend::readNativeTexture(uint32_t id) {
    const auto& info = textures.at(id);
    std::vector<uint8_t> data(static_cast<size_t>(info.width)*info.height*info.channels);
    oglReadTextureData(id,info.channels,info.stencil,data);
    return data;
}
void Backend::updateTexture(uint32_t id,const std::vector<uint8_t>& data,int w,int h,int channels) {
    auto& info = textures.at(id);
    info.width=w; info.height=h;
    setNativeTextureData(id,data,channels);
}
void Backend::setTextureParams(uint32_t id,nicxlive::core::Filtering filter,nicxlive::core::Wrapping wrap,float anisotropy) {
    setNativeTextureFiltering(id,filter,textures.at(id).mipmaps);
    setNativeTextureWrapping(id,wrap);
    setNativeTextureAnisotropy(id,anisotropy);
}
void Backend::disposeTexture(uint32_t id) { textures.erase(id); oglDeleteTextureHandle(id); }
bool Backend::useNativeMultistageBlend(BlendMode mode) { return inUseMultistageBlending(mode); }
std::unique_ptr<nicxlive::core::RenderCommandEmitter> Backend::createNativeCommandEmitter() { return std::make_unique<RenderQueue>(); }
void Backend::beginDynamicComposite(const DynamicCompositePass& pass) {
    composites.push_back(pass);
    oglBeginDynamicComposite(composites.back());
}
void Backend::endDynamicComposite(const DynamicCompositePass& pass) {
    if (composites.empty() || composites.back().surface != pass.surface)
        throw std::logic_error("Unbalanced dynamic composite pass");
    oglEndDynamicComposite(composites.back());
    composites.pop_back();
}
void Backend::destroyDynamicComposite(const std::shared_ptr<DynamicCompositeSurface>& surface) { oglDestroyDynamicComposite(surface); }
void Backend::drawPartPacket(const PartDrawPacket& packet) { oglDrawPartPacket(packet); }
void Backend::resizeViewportTargets(int w,int h) { oglResizeViewport(w,h); }
void Backend::dumpViewport(std::vector<uint8_t>& data,int w,int h) { oglDumpViewport(data,w,h); }
uint32_t Backend::renderImageHandle() { return oglGetRenderImage(); }
uint32_t Backend::framebufferHandle() { return oglGetFramebuffer(); }
uint32_t Backend::mainAlbedoHandle() { return oglGetMainAlbedo(); }
uint32_t Backend::mainEmissiveHandle() { return oglGetMainEmissive(); }
uint32_t Backend::mainBumpHandle() { return oglGetMainBump(); }
uint32_t Backend::blendFramebufferHandle() { return oglGetBlendFramebuffer(); }
uint32_t Backend::blendAlbedoHandle() { return oglGetBlendAlbedo(); }
uint32_t Backend::blendEmissiveHandle() { return oglGetBlendEmissive(); }
uint32_t Backend::blendBumpHandle() { return oglGetBlendBump(); }
void Backend::beginScene() { oglBeginScene(); }
void Backend::endScene() { oglEndScene(); }
void Backend::postProcessScene() { oglPostProcessScene(); }
void Backend::addBasicLightingPostProcess() { oglAddBasicLightingPostProcess(); }
void Backend::setDifferenceAggregationEnabled(bool value) { oglSetDifferenceAggregationEnabled(value); }
bool Backend::isDifferenceAggregationEnabled() { return oglIsDifferenceAggregationEnabled(); }
void Backend::setDifferenceAggregationRegion(const DifferenceEvaluationRegion& region) { oglSetDifferenceAggregationRegion(region); }
DifferenceEvaluationRegion Backend::getDifferenceAggregationRegion() { return oglGetDifferenceAggregationRegion(); }
bool Backend::evaluateDifferenceAggregation(uint32_t texture,int w,int h) { return oglEvaluateDifferenceAggregation(texture,w,h); }
bool Backend::fetchDifferenceAggregationResult(DifferenceEvaluationResult& result) { return oglFetchDifferenceAggregationResult(result); }
void Backend::drawTextureAtPart(const Texture& tex,const std::shared_ptr<Part>& part) { oglDrawTextureAtPart(&tex,part); }
void Backend::drawTextureAtPosition(const Texture& tex,const vec2& pos,float opacity,const vec3& color,const vec3& screen) { oglDrawTextureAtPosition(&tex,pos,opacity,color,screen); }
void Backend::drawTextureAtRect(const Texture& tex,const Rect& area,const Rect& uv,float opacity,const vec3& color,const vec3& screen,void* shader,void* camera) {
    oglDrawTextureAtRect(&tex,area,uv,opacity,color,screen,shader ? *static_cast<Shader*>(shader) : Shader{},static_cast<Camera*>(camera));
}
}
