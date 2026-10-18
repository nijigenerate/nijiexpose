#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace niji {
inline void sdl_check(bool success, const char *operation) {
  if (!success) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}
class SdlRuntime {
public:
  SdlRuntime(bool camera, bool video) {
    SDL_SetMainReady();
    sdl_check(SDL_Init(SDL_INIT_EVENTS | (camera ? SDL_INIT_CAMERA : 0) | (video ? SDL_INIT_VIDEO : 0)), "initialize SDL");
  }
  ~SdlRuntime() { SDL_Quit(); }
  SdlRuntime(const SdlRuntime &) = delete;
  SdlRuntime &operator=(const SdlRuntime &) = delete;
};
using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
inline Surface surface(SDL_Surface *value) {
  sdl_check(value != nullptr, "create RGB surface");
  return Surface(value, SDL_DestroySurface);
}
inline void copy_rgb(SDL_Surface *source, int max_width, int max_height,
                     std::vector<uint8_t> &pixels, int &width, int &height) {
  if (!source || source->w < 1 || source->h < 1 || source->w > 8192 || source->h > 8192)
    throw std::runtime_error("Invalid camera frame size");
  auto rgb = surface(SDL_ConvertSurfaceAndColorspace(source, SDL_PIXELFORMAT_RGB24, nullptr, SDL_COLORSPACE_SRGB, 0));
  const double scale = std::min({1.0, double(max_width)/rgb->w, double(max_height)/rgb->h});
  if (scale < 1.0)
    rgb = surface(SDL_ScaleSurface(rgb.get(), std::max(1, static_cast<int>(rgb->w*scale)),
                                  std::max(1, static_cast<int>(rgb->h*scale)), SDL_SCALEMODE_LINEAR));
  width = rgb->w; height = rgb->h;
  pixels.resize(static_cast<size_t>(width)*height*3);
  for (int y = 0; y < height; ++y)
    std::memcpy(pixels.data()+static_cast<size_t>(y)*width*3,
                static_cast<const uint8_t *>(rgb->pixels)+static_cast<size_t>(y)*rgb->pitch,
                static_cast<size_t>(width)*3);
}
inline std::vector<uint8_t> rgb_pixels(const uint8_t *data, int width, int height,
                                     int channels, bool flip = false) {
  if (!data || width < 1 || height < 1 || width > 8192 || height > 8192 ||
      (channels != 3 && channels != 4))
    throw std::runtime_error("Expected an RGB or RGBA image");
  std::vector<uint8_t> result(static_cast<size_t>(width) * height * 3);
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) {
      const auto source = (static_cast<size_t>(y) * width + (flip ? width - 1 - x : x)) * channels;
      const auto destination = (static_cast<size_t>(y) * width + x) * 3;
      std::copy_n(data + source, 3, result.data() + destination);
    }
  return result;
}
struct Camera {
  int id = 0;
  std::string name, path, uuid;
  SDL_CameraID instance = 0;
};
inline std::vector<Camera> cameras() {
  int count = 0;
  SDL_CameraID *ids = SDL_GetCameras(&count);
  sdl_check(ids != nullptr, "enumerate cameras");
  std::unique_ptr<SDL_CameraID, decltype(&SDL_free)> owned(ids, SDL_free);
  std::vector<Camera> result;
  for (int i = 0; i < count; ++i) {
    const char *name = SDL_GetCameraName(ids[i]);
    sdl_check(name != nullptr, "get camera name");
    result.push_back({i, name, "", "", ids[i]});
  }
  return result;
}
} // namespace niji
