#pragma once
#include "sdl_media.hpp"
#include "stream.hpp"
#include "timing.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
namespace niji {
class CameraReader {
  SDL_Camera *camera_ = nullptr;
  int width_, height_;
public:
  CameraReader(const std::string &input, int width, int height, double fps)
      : width_(width), height_(height) {
    const auto devices = cameras();
    size_t consumed = 0;
    int index = -1;
    try { index = std::stoi(input, &consumed); } catch (const std::exception &) {}
    SDL_CameraID instance = 0;
    if (consumed == input.size() && index >= 0 && static_cast<size_t>(index) < devices.size())
      instance = devices[static_cast<size_t>(index)].instance;
    else for (const auto &device : devices) if (device.name == input) { instance = device.instance; break; }
    if (!instance) throw std::runtime_error("Camera not found: " + input);
    const SDL_CameraSpec spec{SDL_PIXELFORMAT_RGB24, SDL_COLORSPACE_SRGB, width, height,
                             static_cast<int>(std::lround(fps*1000)), 1000};
    camera_ = SDL_OpenCamera(instance, &spec);
    sdl_check(camera_ != nullptr, "open SDL camera");
    std::ostringstream message;
    message << "capture backend=SDL3/" << SDL_GetCurrentCameraDriver()
            << " requested_fps=" << fps << " width=" << width << " height=" << height << '\n';
    std::cerr << message.str();
  }
  ~CameraReader() { if (camera_) SDL_CloseCamera(camera_); }
  CameraReader(const CameraReader &) = delete;
  CameraReader &operator=(const CameraReader &) = delete;
  bool next(std::vector<uint8_t> &pixels, int &width, int &height,
            std::chrono::steady_clock::time_point *captured_at = nullptr,
            const std::function<bool()> &stopped = {}) {
    while (!stopped || !stopped()) {
      // SDL can still be resolving permission after OpenCamera succeeds,
      // including when macOS already has authorization for this application.
      const auto permission = SDL_GetCameraPermissionState(camera_);
      if (permission < 0)
        throw std::runtime_error("Camera permission denied");
      if (permission == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
      }
      SDL_ClearError();
      Uint64 timestamp = 0;
      SDL_Surface *frame = SDL_AcquireCameraFrame(camera_, &timestamp);
      if (!frame) {
        if (*SDL_GetError()) throw std::runtime_error(std::string("Acquire camera frame: ")+SDL_GetError());
        if (SDL_GetCameraPermissionState(camera_) < 0)
          throw std::runtime_error("Camera permission denied");
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue;
      }
      struct Release { SDL_Camera *camera; SDL_Surface *frame; ~Release() { SDL_ReleaseCameraFrame(camera,frame); } } release{camera_,frame};
      if (captured_at) *captured_at = std::chrono::steady_clock::now();
      copy_rgb(frame, width_, height_, pixels, width, height);
      return true;
    }
    return false;
  }
};
struct CameraFrame {
  std::vector<uint8_t> rgb;
  int width = 0, height = 0;
  std::uint64_t sequence = 0;
  std::chrono::steady_clock::time_point captured_at;
};
class AsyncCameraReader {
  std::mutex timing_mutex_;
  CaptureTiming timing_;
  WorkerStream<CameraFrame> stream_;
public:
  AsyncCameraReader(std::string input, int width, int height, double fps)
      : stream_(Delivery::latest,
          [this,input=std::move(input),width,height,fps](auto &stream) {
            CameraReader reader(input,width,height,fps);
            std::uint64_t sequence = 0;
            std::optional<std::chrono::steady_clock::time_point> previous;
            while (!stream.stopped()) {
              CameraFrame frame;
              const auto before = std::chrono::steady_clock::now();
              if (!reader.next(frame.rgb,frame.width,frame.height,&frame.captured_at,[&] { return stream.stopped(); })) break;
              {
                std::lock_guard lock(timing_mutex_);
                timing_.reads.add(std::chrono::duration<double,std::milli>(frame.captured_at-before).count());
                if (previous) timing_.intervals.add(std::chrono::duration<double,std::milli>(frame.captured_at-*previous).count());
              }
              previous = frame.captured_at; frame.sequence = sequence++;
              if (stream.stopped() || !stream.publish(std::move(frame))) break;
            }
          }) {}
  ReadStatus next(CameraFrame &frame, std::chrono::milliseconds timeout) { return stream_.next(frame,timeout); }
  void request_stop() { stream_.request_stop(); }
  void join() { stream_.join(); }
  std::uint64_t captured() const { return stream_.published(); }
  std::uint64_t dropped() const { return stream_.dropped(); }
  CaptureTiming take_timing() { std::lock_guard lock(timing_mutex_); auto value=timing_; timing_={}; return value; }
};
} // namespace niji
