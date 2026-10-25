#pragma once
// Test-only D3D11 receiver: opens the actual published Spout handle and reads
// its pixels under the SDK's named texture-access mutex. No sender internals.
#include "spout.hpp"
#include <cstring>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <wrl/client.h>
namespace spout_test {
using Microsoft::WRL::ComPtr;
inline void check(HRESULT hr) {
  if (FAILED(hr))
    throw std::runtime_error("D3D11 receiver error: " + std::to_string(hr));
}
struct Handle {
  HANDLE value{};
  bool locked = false;
  ~Handle() {
    if (locked)
      ReleaseMutex(value);
    if (value)
      CloseHandle(value);
  }
};
inline std::vector<uint8_t> receive(const char *name, const uint8_t *luid,
                                    uint32_t expectedWidth,
                                    uint32_t expectedHeight,
                                    HANDLE expectedHandle) {
  if (!nijiexpose::spout::isSpout2Loaded() &&
      nijiexpose::spout::loadSpout2() !=
          nijiexpose::spout::Spout2Support::spout2)
    throw std::runtime_error("Independent Spout receiver DLL unavailable");
  static auto observer = nijiexpose::spout::spGetSpout();
  uint32_t w{}, h{};
  HANDLE shared{};
  DWORD format{};
  // Spout's wire format stores KMT handles as uint32_t; its legacy DLL
  // sign-extends them with LongToHandle on x64. Compare that wire value.
  if (!nijiexpose::spout::spGetSenderInfo(observer, name, w, h, shared,
                                          format) ||
      !shared || HandleToULong(shared) != HandleToULong(expectedHandle) || w != expectedWidth ||
      h != expectedHeight || format != 28)
    throw std::runtime_error("Published Spout texture metadata mismatch: handle=" +
      std::to_string(reinterpret_cast<uintptr_t>(shared)) + " expected=" +
      std::to_string(reinterpret_cast<uintptr_t>(expectedHandle)) + " size=" +
      std::to_string(w) + "x" + std::to_string(h) + " expected=" +
      std::to_string(expectedWidth) + "x" + std::to_string(expectedHeight) +
      " format=" + std::to_string(format));
  ComPtr<IDXGIFactory1> factory;
  check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
  ComPtr<IDXGIAdapter1> adapter;
  for (UINT i = 0;; ++i) {
    ComPtr<IDXGIAdapter1> candidate;
    if (factory->EnumAdapters1(i, &candidate) == DXGI_ERROR_NOT_FOUND)
      throw std::runtime_error("Spout receiver cannot find Vulkan adapter");
    DXGI_ADAPTER_DESC1 desc{};
    check(candidate->GetDesc1(&desc));
    if (!std::memcmp(&desc.AdapterLuid, luid, 8)) {
      adapter = candidate;
      break;
    }
  }
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> context;
  check(D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
                          nullptr, 0, D3D11_SDK_VERSION, &device, nullptr,
                          &context));
  ComPtr<ID3D11Texture2D> texture;
  check(device->OpenSharedResource(shared, IID_PPV_ARGS(&texture)));
  D3D11_TEXTURE2D_DESC desc{};
  texture->GetDesc(&desc);
  desc.Usage = D3D11_USAGE_STAGING;
  desc.BindFlags = desc.MiscFlags = 0;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  ComPtr<ID3D11Texture2D> staging;
  check(device->CreateTexture2D(&desc, nullptr, &staging));
  Handle access;
  access.value = OpenMutexA(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE,
                            (std::string(name) + "_SpoutAccessMutex").c_str());
  if (!access.value || WaitForSingleObject(access.value, 5000) != WAIT_OBJECT_0)
    throw std::runtime_error("Cannot lock Spout sender texture");
  access.locked = true;
  context->CopyResource(staging.Get(), texture.Get());
  D3D11_MAPPED_SUBRESOURCE mapped{};
  check(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped));
  std::vector<uint8_t> pixels(size_t(w) * h * 4);
  for (uint32_t y = 0; y < h; ++y)
    std::memcpy(pixels.data() + size_t(y) * w * 4,
                static_cast<uint8_t *>(mapped.pData) +
                    size_t(y) * mapped.RowPitch,
                size_t(w) * 4);
  context->Unmap(staging.Get(), 0);
  return pixels;
}
} // namespace spout_test
