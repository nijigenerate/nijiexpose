// nijimi OpenGL host: resource operations and playback of nicxlive command queues.
#include "backend.hpp"
#include "probes.hpp"
#include "runtime.hpp"
#include "drawable.hpp"
#include "debug.hpp"
#include "part.hpp"
#include "mask.hpp"
#include "blending.hpp"
#include "draw_texture.hpp"
#include <core/render/backend_queue.hpp>
#include <core/render/shared_deform_buffer.hpp>
#include "difference.hpp"
#include <algorithm>
#include <stdexcept>
namespace nijiexpose::ogl {
void Backend::initializeRenderer() {
    resourceCallbacks.userData=this;
    resourceCallbacks.createTexture=[](int w,int h,int channels,int,int,bool,bool stencil,void* user)->size_t {
        auto& self=*static_cast<Backend*>(user);
        GLuint id{}; oglCreateTextureHandle(id);
        self.textures.emplace(id,TextureInfo{w,h,channels,stencil,false});
        oglUploadTextureData(id,w,h,channels,channels,stencil,{});
        self.setTextureParams(id,nicxlive::core::Filtering::Linear,nicxlive::core::Wrapping::Clamp,oglMaxTextureAnisotropy()/2.0f);
        return id;
    };
    resourceCallbacks.updateTexture=[](size_t handle,const uint8_t* data,size_t length,int w,int h,int channels,void* user) {
        auto& self=*static_cast<Backend*>(user);
        auto it=self.textures.find(static_cast<uint32_t>(handle));
        if(it==self.textures.end() || !data || w<=0 || h<=0 || channels<1 || channels>4)return;
        const auto expected=static_cast<size_t>(w)*h*channels;
        if(length<expected)return;
        std::vector<uint8_t> pixels(data,data+expected);
        const auto& info=it->second;
        oglUploadTextureData(static_cast<uint32_t>(handle),w,h,channels,info.channels,info.stencil,pixels);
        oglRecordCpuTexture(static_cast<uint32_t>(handle),w,h,channels,pixels);
    };
    resourceCallbacks.releaseTexture=[](size_t handle,void* user) {
        auto& self=*static_cast<Backend*>(user);
        auto id=static_cast<uint32_t>(handle);
        if(self.textures.erase(id)) { oglReleaseCpuTexture(id); oglDeleteTextureHandle(id); }
    };
    loadApi();
    oglInitRenderer();
    oglInitDrawableBackend();
    oglInitPartBackendResources();
    oglInitMaskBackend();
}
void Backend::initializeDrawableResources() { oglInitDrawableBackend(); }
void Backend::bindDrawableVao() { oglBindDrawableVao(); }
void Backend::createDrawableBuffers(uint32_t& id) { oglCreateDrawableBuffers(id); }
void Backend::disposeDrawableBuffers(uint32_t id) { oglDisposeDrawableBuffers(id); }
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
    auto id=static_cast<uint32_t>(resourceCallbacks.createTexture(w,h,channels,1,0,false,stencil,resourceCallbacks.userData));
    updateTexture(id,data,w,h,channels);
    return id;
}
void Backend::updateTexture(uint32_t id,const std::vector<uint8_t>& data,int w,int h,int channels) {
    resourceCallbacks.updateTexture(id,data.data(),data.size(),w,h,channels,resourceCallbacks.userData);
}

void Backend::setTextureParams(uint32_t id,nicxlive::core::Filtering filter,nicxlive::core::Wrapping wrap,float anisotropy) {
    oglApplyTextureFiltering(id,filter==nicxlive::core::Filtering::Linear ? Filtering::Linear : Filtering::Nearest,false);
    oglApplyTextureWrapping(id,static_cast<Wrapping>(wrap));
    oglApplyTextureAnisotropy(id,std::clamp(anisotropy,1.f,oglMaxTextureAnisotropy()));
}
std::vector<uint8_t> Backend::readTexture(uint32_t id) {
    const auto& info=textures.at(id);
    std::vector<uint8_t> data(static_cast<size_t>(info.width)*info.height*info.channels);
    oglReadTextureData(id,info.channels,info.stencil,data);
    return data;
}
void Backend::disposeTexture(uint32_t id) { resourceCallbacks.releaseTexture(id,resourceCallbacks.userData); }
void Backend::drawPuppet(nicxlive::core::Puppet& puppet) {
    using namespace nicxlive::core;
    using namespace nicxlive::core::render;
    // The core produces its standard command queue; OpenGL only consumes it,
    // as renderCommands does in nijimi/source/render/opengl/opengl_backend.d.
    auto host=getCurrentRenderBackend();
    auto queue=std::make_shared<QueueRenderBackend>();
    puppet.setRenderBackend(queue);
    setCurrentRenderBackend(host);
    puppet.draw();
    uploadSharedVertexBuffer(sharedVertexBufferData());
    uploadSharedUvBuffer(sharedUvBufferData());
    uploadSharedDeformBuffer(sharedDeformBufferData());
    sharedVertexMarkUploaded(); sharedUvMarkUploaded(); sharedDeformMarkUploaded();
    for(const auto& cmd:queue->queue) {
        switch(cmd.kind) {
            case RenderCommandKind::DrawPart: drawPartPacket(cmd.partPacket); break;
            case RenderCommandKind::BeginMask: beginMask(cmd.usesStencil); break;
            case RenderCommandKind::ApplyMask: applyMask(cmd.maskApplyPacket); break;
            case RenderCommandKind::BeginMaskContent: beginMaskContent(); break;
            case RenderCommandKind::EndMask: endMask(); break;
            case RenderCommandKind::BeginDynamicComposite: beginDynamicComposite(cmd.dynamicPass); break;
            case RenderCommandKind::EndDynamicComposite: endDynamicComposite(cmd.dynamicPass); break;
        }
    }
    queue->clear();
}
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
