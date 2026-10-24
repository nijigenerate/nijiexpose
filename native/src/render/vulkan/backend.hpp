#pragma once
#include "context.hpp"
#include <array>
#include <core/puppet.hpp>
#include <core/render/common.hpp>
#include <map>
#include <memory>

namespace nijiexpose::vulkan {
class Backend final : public nicxlive::core::RenderBackend {
  struct Impl;
  std::unique_ptr<Impl> p;

public:
  Backend();
  ~Backend() override;
  void shutdown();
  void initializeRenderer() override;
  void createDrawableBuffers(uint32_t &) override;
  void disposeDrawableBuffers(uint32_t) override;
  void uploadDrawableIndices(uint32_t, const std::vector<uint16_t> &) override;
  uint32_t createTexture(const std::vector<uint8_t> &, int, int, int,
                         bool) override;
  void updateTexture(uint32_t, const std::vector<uint8_t> &, int, int,
                     int) override;
  void setTextureParams(uint32_t, nicxlive::core::Filtering,
                        nicxlive::core::Wrapping, float) override;
  void disposeTexture(uint32_t) override;
  std::vector<uint8_t> readTexture(uint32_t);
  ImTextureID uiTexture(uint32_t);
  void drawPuppet(nicxlive::core::Puppet &);
  void drawPartPacket(const nicxlive::core::nodes::PartDrawPacket &) override;
  void beginMask(bool) override;
  void applyMask(const MaskApplyPacket &) override;
  void beginMaskContent() override;
  void endMask() override;
  void
  beginDynamicComposite(const nicxlive::core::DynamicCompositePass &) override;
  void
  endDynamicComposite(const nicxlive::core::DynamicCompositePass &) override;
  void destroyDynamicComposite(
      const std::shared_ptr<nicxlive::core::DynamicCompositeSurface> &)
      override;
  void resizeViewportTargets(int, int) override;
  void beginScene() override;
  void endScene() override;
  void postProcessScene() override;
  void addBasicLightingPostProcess() override;
  void dumpViewport(std::vector<uint8_t> &, int, int) override;
  uint32_t renderImageHandle() override;
  uint32_t mainAlbedoHandle() override;
  uint32_t mainEmissiveHandle() override;
  uint32_t mainBumpHandle() override;
  void render();
  Image &sceneImage();
  ImTextureID overlayBlurTexture();
  void drawSprites(uint32_t, const float *, size_t,
                   const nicxlive::core::nodes::Mat4 &);
};
Backend &backend();
ImTextureID uiTexture(uint32_t);
} // namespace nijiexpose::vulkan
