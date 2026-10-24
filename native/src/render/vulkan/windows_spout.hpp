#pragma once
#include <cstdint>
#include <memory>
#include <vulkan/vulkan.h>
namespace nijiexpose::vulkan {
struct Image;
// Windows-only Spout sender. The published D3D11 texture is imported into
// Vulkan.
class SpoutBridge {
  friend class Context;
  void shutdown();
  struct Impl;
  std::unique_ptr<Impl> p;

public:
  SpoutBridge();
  ~SpoutBridge();
  Image createImage(uint32_t width, uint32_t height);
  bool beginFrame();
  void cancelFrame();
  void releaseImage(VkCommandBuffer command, Image &image);
  bool send(Image &image);
  const char *name() const;
};
} // namespace nijiexpose::vulkan
