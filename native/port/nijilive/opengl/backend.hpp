#pragma once
#include "nicxlive/native_backend.hpp"
#include "dynamic_composite.hpp"
#include <unordered_map>
namespace nijiexpose::ogl {
class Backend final : public nicxlive::core::RenderBackend, public nicxlive::core::NativeBackend {
    struct TextureInfo { int width, height, channels; bool stencil, mipmaps; };
    std::unordered_map<uint32_t, TextureInfo> textures;
    std::vector<DynamicCompositePass> composites;
public:
    void initializeRenderer() override;
    void initializeDrawableResources() override;
    void bindDrawableVao() override;
    void createDrawableBuffers(uint32_t&) override;
    void uploadDrawableIndices(uint32_t, const std::vector<uint16_t>&) override;
    void uploadSharedVertexBuffer(const nicxlive::core::common::Vec2Array&) override;
    void uploadSharedUvBuffer(const nicxlive::core::common::Vec2Array&) override;
    void uploadSharedDeformBuffer(const nicxlive::core::common::Vec2Array&) override;
    void drawDrawableElements(uint32_t, size_t) override;
    void setDebugPointSize(float) override;
    void setDebugLineWidth(float) override;
    void uploadDebugBuffer(const std::vector<vec3>&, const std::vector<uint16_t>&) override;
    void drawDebugPoints(const vec4&, const mat4&) override;
    void drawDebugLines(const vec4&, const mat4&) override;
    void beginMask(bool) override;
    void applyMask(const MaskApplyPacket&) override;
    void beginMaskContent() override;
    void endMask() override;
    uint32_t createTexture(const std::vector<uint8_t>&, int, int, int, bool) override;
    void updateTexture(uint32_t, const std::vector<uint8_t>&, int, int, int) override;
    void setTextureParams(uint32_t, nicxlive::core::Filtering, nicxlive::core::Wrapping, float) override;
    void disposeTexture(uint32_t) override;
    uint32_t createNativeTexture(const std::vector<uint8_t>&, int, int, int, int, bool, bool) override;
    void setNativeTextureFiltering(uint32_t, nicxlive::core::Filtering, bool) override;
    void setNativeTextureWrapping(uint32_t, nicxlive::core::Wrapping) override;
    void setNativeTextureAnisotropy(uint32_t, float) override;
    void setNativeTextureData(uint32_t, const std::vector<uint8_t>&, int) override;
    std::vector<uint8_t> readNativeTexture(uint32_t) override;
    bool useNativeMultistageBlend(nicxlive::core::nodes::BlendMode) override;
    std::unique_ptr<nicxlive::core::RenderCommandEmitter> createNativeCommandEmitter() override;
    void beginDynamicComposite(const DynamicCompositePass&) override;
    void endDynamicComposite(const DynamicCompositePass&) override;
    void destroyDynamicComposite(const std::shared_ptr<DynamicCompositeSurface>&) override;
    void drawPartPacket(const nicxlive::core::nodes::PartDrawPacket&) override;
    void resizeViewportTargets(int, int) override;
    void dumpViewport(std::vector<uint8_t>&, int, int) override;
    uint32_t renderImageHandle() override;
    uint32_t framebufferHandle() override;
    uint32_t mainAlbedoHandle() override;
    uint32_t mainEmissiveHandle() override;
    uint32_t mainBumpHandle() override;
    uint32_t blendFramebufferHandle() override;
    uint32_t blendAlbedoHandle() override;
    uint32_t blendEmissiveHandle() override;
    uint32_t blendBumpHandle() override;
    void beginScene() override;
    void endScene() override;
    void postProcessScene() override;
    void addBasicLightingPostProcess() override;
    void setDifferenceAggregationEnabled(bool) override;
    bool isDifferenceAggregationEnabled() override;
    void setDifferenceAggregationRegion(const nicxlive::core::DifferenceEvaluationRegion&) override;
    nicxlive::core::DifferenceEvaluationRegion getDifferenceAggregationRegion() override;
    bool evaluateDifferenceAggregation(uint32_t,int,int) override;
    bool fetchDifferenceAggregationResult(nicxlive::core::DifferenceEvaluationResult&) override;
    void drawTextureAtPart(const nicxlive::core::Texture&, const std::shared_ptr<nicxlive::core::nodes::Part>&) override;
    void drawTextureAtPosition(const nicxlive::core::Texture&, const vec2&, float, const vec3&, const vec3&) override;
    void drawTextureAtRect(const nicxlive::core::Texture&, const nicxlive::core::Rect&, const nicxlive::core::Rect&, float, const vec3&, const vec3&, void*, void*) override;
};
}
