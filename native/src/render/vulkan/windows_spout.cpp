#include "windows_spout.hpp"
#include "context.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <SpoutFrameCount.h>
#include <SpoutSenderNames.h>
#include <cstring>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan_win32.h>
#include <windows.h>
#include <wrl/client.h>

namespace nijiexpose::vulkan {
namespace {
using Microsoft::WRL::ComPtr;
void dxCheck(HRESULT result) {
  if (FAILED(result))
    throw std::runtime_error("Spout D3D11 error " + std::to_string(result));
}
} // namespace
struct SpoutBridge::Impl {
  Context &c = context();
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> immediate;
  ComPtr<ID3D11Texture2D> texture;
  spoutSenderNames names;
  spoutFrameCount frame;
  char sender[256] = "nijiexpose";
  bool registered = false;
  bool locked = false;
  void *publishedHandle{};
  // Keep the previously published KMT allocation valid until metadata points
  // at its replacement, including while the renderer rebuilds its framebuffer.
  std::shared_ptr<void> publishedOwner;
  void initialize() {
    if (!c.externalWin32Supported)
      throw std::runtime_error("Vulkan Win32 memory import is unavailable");
    VkPhysicalDeviceIDProperties id{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
    VkPhysicalDeviceProperties2 properties{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    properties.pNext = &id;
    vkGetPhysicalDeviceProperties2(c.physical, &properties);
    if (!id.deviceLUIDValid)
      throw std::runtime_error("Vulkan adapter LUID unavailable");
    ComPtr<IDXGIFactory1> factory;
    dxCheck(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0;; ++i) {
      ComPtr<IDXGIAdapter1> candidate;
      auto result = factory->EnumAdapters1(i, &candidate);
      if (result == DXGI_ERROR_NOT_FOUND)
        throw std::runtime_error("Spout cannot find Vulkan's D3D11 adapter");
      dxCheck(result);
      DXGI_ADAPTER_DESC1 desc{};
      dxCheck(candidate->GetDesc1(&desc));
      if (!std::memcmp(&desc.AdapterLuid, id.deviceLUID, sizeof(LUID))) {
        adapter = candidate;
        break;
      }
    }
    dxCheck(D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                              0, nullptr, 0, D3D11_SDK_VERSION, &device,
                              nullptr, &immediate));
    for (int i = 1; names.FindSenderName(sender); ++i)
      sprintf_s(sender, "nijiexpose (%d)", i);
    if (!names.RegisterSenderName(sender, true))
      throw std::runtime_error("Cannot reserve Spout sender name");
    registered = true;
    if (!frame.CreateAccessMutex(sender))
      throw std::runtime_error("Cannot create Spout access mutex");
    frame.EnableFrameCount(sender);
  }
  Image createImage(uint32_t w, uint32_t h) {
    Image shared;
    try {
      constexpr auto handleType =
          VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT;
      VkPhysicalDeviceExternalImageFormatInfo external{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO};
      external.handleType = handleType;
      VkPhysicalDeviceImageFormatInfo2 query{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2};
      query.pNext = &external;
      query.format = VK_FORMAT_R8G8B8A8_UNORM;
      query.type = VK_IMAGE_TYPE_2D;
      query.tiling = VK_IMAGE_TILING_OPTIMAL;
      query.usage =
          VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      VkExternalImageFormatProperties ep{
          VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES};
      VkImageFormatProperties2 properties{
          VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2};
      properties.pNext = &ep;
      check(vkGetPhysicalDeviceImageFormatProperties2(c.physical, &query,
                                                      &properties));
      if (!(ep.externalMemoryProperties.externalMemoryFeatures &
            VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT))
        throw std::runtime_error(
            "Spout RGBA8 KMT texture cannot be imported into Vulkan");
      D3D11_TEXTURE2D_DESC desc{};
      desc.Width = w;
      desc.Height = h;
      desc.MipLevels = desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.SampleDesc.Count = 1;
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
      desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
      dxCheck(device->CreateTexture2D(&desc, nullptr, &texture));
      ComPtr<IDXGIResource> resource;
      dxCheck(texture.As(&resource));
      HANDLE handle{};
      dxCheck(resource->GetSharedHandle(
          &handle)); // KMT handle: do not CloseHandle.
      immediate->Flush();
      VkExternalMemoryImageCreateInfo ext{
          VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
      ext.handleTypes = handleType;
      VkImageCreateInfo im{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
      im.pNext = &ext;
      im.imageType = query.type;
      im.format = query.format;
      im.extent = {w, h, 1};
      im.mipLevels = im.arrayLayers = 1;
      im.samples = VK_SAMPLE_COUNT_1_BIT;
      im.tiling = query.tiling;
      im.usage = query.usage;
      check(vkCreateImage(c.device, &im, nullptr, &shared.handle));
      VkMemoryRequirements req{};
      vkGetImageMemoryRequirements(c.device, shared.handle, &req);
      auto getProperties =
          reinterpret_cast<PFN_vkGetMemoryWin32HandlePropertiesKHR>(
              vkGetDeviceProcAddr(c.device,
                                  "vkGetMemoryWin32HandlePropertiesKHR"));
      if (!getProperties)
        throw std::runtime_error("Vulkan Win32 import properties entry absent");
      VkMemoryWin32HandlePropertiesKHR hp{
          VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR};
      check(getProperties(c.device, handleType, handle, &hp));
      VkImportMemoryWin32HandleInfoKHR import{
          VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR};
      import.handleType = handleType;
      import.handle = handle;
      VkMemoryDedicatedAllocateInfo dedicated{
          VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
      dedicated.pNext = &import;
      dedicated.image = shared.handle;
      VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
      alloc.pNext = &dedicated;
      alloc.allocationSize = req.size;
      alloc.memoryTypeIndex =
          c.memoryType(req.memoryTypeBits & hp.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      check(vkAllocateMemory(c.device, &alloc, nullptr, &shared.memory));
      check(vkBindImageMemory(c.device, shared.handle, shared.memory, 0));
      shared.width = w;
      shared.height = h;
      VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
      view.image = shared.handle;
      view.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view.format = shared.format;
      view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
      check(vkCreateImageView(c.device, &view, nullptr, &shared.view));
      shared.externalOwner = std::make_shared<ComPtr<ID3D11Texture2D>>(texture);
      shared.sharedHandle = handle;
      texture.Reset();
      return shared;
    } catch (...) {
      c.destroy(shared);
      texture.Reset();
      throw;
    }
  }
  bool beginFrame() {
    if (locked)
      throw std::logic_error("Spout frame already acquired");
    locked = frame.CheckAccess();
    return locked;
  }
  void cancelFrame() {
    if (locked) {
      frame.AllowAccess();
      locked = false;
    }
  }
  void releaseImage(VkCommandBuffer command, Image &image) {
    if (!image.externalOwner)
      return;
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.image = image.handle;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    barrier.oldLayout = image.layout;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = c.family;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
    barrier.srcAccessMask =
        VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);
    image.layout = VK_IMAGE_LAYOUT_GENERAL;
    image.externalOwned = true;
  }
  bool send(Image &image) {
    if (!locked || !image.externalOwner)
      return false;
    // The same image was the model's render target. No copy or extra
    // submission. The named mutex must cover GPU completion, not just command
    // recording.
    check(vkWaitForFences(c.device, 1, &c.lastSubmit_, VK_TRUE, UINT64_MAX));
    if (publishedHandle != image.sharedHandle) {
      if (!names.UpdateSender(sender, image.width, image.height,
                              image.sharedHandle, DXGI_FORMAT_R8G8B8A8_UNORM))
        throw std::runtime_error("Cannot publish Spout render target");
      publishedHandle = image.sharedHandle;
      publishedOwner = image.externalOwner;
    }
    frame.SetNewFrame();
    cancelFrame();
    return true;
  }
  ~Impl() {
    if (c.device)
      vkDeviceWaitIdle(c.device);
    cancelFrame();
    if (registered)
      names.ReleaseSenderName(sender);
    frame.CleanupFrameCount();
    frame.CloseAccessMutex();
  }
};
SpoutBridge::SpoutBridge() : p(std::make_unique<Impl>()) {
  p->initialize();
  p->c.spoutBridge_ = this;
}
SpoutBridge::~SpoutBridge() { shutdown(); }
void SpoutBridge::shutdown() {
  if (p) {
    p->c.spoutBridge_ = nullptr;
    p.reset();
  }
}
Image SpoutBridge::createImage(uint32_t w, uint32_t h) {
  return p->createImage(w, h);
}
bool SpoutBridge::beginFrame() { return p && p->beginFrame(); }
void SpoutBridge::cancelFrame() {
  if (p)
    p->cancelFrame();
}
void SpoutBridge::releaseImage(VkCommandBuffer cmd, Image &image) {
  p->releaseImage(cmd, image);
}
bool SpoutBridge::send(Image &image) { return p && p->send(image); }
const char *SpoutBridge::name() const { return p ? p->sender : ""; }
} // namespace nijiexpose::vulkan
