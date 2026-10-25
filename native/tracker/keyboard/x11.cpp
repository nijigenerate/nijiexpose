#include "internal.hpp"
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/record.h>
#include <cstdlib>
#include <cstring>
#include <poll.h>

namespace niji::keyboard {
class X11Backend final : public Backend {
  std::shared_ptr<Sink> sink_;
  Display *control_ = nullptr;
  Display *data_ = nullptr;
  XRecordContext context_ = 0;
  bool enabled_ = false;
  static void callback(XPointer pointer, XRecordInterceptData *record) {
    auto *self = reinterpret_cast<X11Backend *>(pointer);
    if (record->category == XRecordFromServer) {
      size_t size = record->data_len * 4;
      for (size_t pos = 0; pos + 32 <= size; pos += 32) {
        const auto *bytes = record->data + pos;
        int type = bytes[0] & 0x7f;
        if (type != KeyPress && type != KeyRelease)
          continue;
        uint16_t state = 0;
        std::memcpy(&state, bytes + 28, 2);
        if (record->client_swapped)
          state = uint16_t((state >> 8) | (state << 8));
        KeySym sym = NoSymbol;
        unsigned consumed = 0;
        XkbLookupKeySym(self->control_, bytes[1], state, &consumed, &sym);
        uint32_t cp = 0;
        if (sym <= 0xff)
          cp = static_cast<uint32_t>(sym);
        else if ((sym & 0xff000000) == 0x01000000)
          cp = static_cast<uint32_t>(sym & 0xffffff);
        self->sink_->physical(bytes[1], type == KeyPress, cp);
      }
    }
    XRecordFreeData(record);
  }
  void cleanup() {
    if (enabled_ && control_) {
      XRecordDisableContext(control_, context_);
      XSync(control_, False);
      enabled_ = false;
    }
    if (context_ && control_) {
      XRecordFreeContext(control_, context_);
      context_ = 0;
    }
    if (data_) {
      XCloseDisplay(data_);
      data_ = nullptr;
    }
    if (control_) {
      XCloseDisplay(control_);
      control_ = nullptr;
    }
  }

public:
  explicit X11Backend(std::shared_ptr<Sink> sink) : sink_(std::move(sink)) {
    const char *session = std::getenv("XDG_SESSION_TYPE");
    if (session && std::string(session) == "wayland")
      throw std::runtime_error("Global keyboard capture on Wayland is not implemented; XWayland is "
                               "not a desktop-wide substitute");
    control_ = XOpenDisplay(nullptr);
    data_ = XOpenDisplay(nullptr);
    if (!control_ || !data_) {
      cleanup();
      throw std::runtime_error("Keyboard capture needs a reachable X11 DISPLAY");
    }
    int major = 0, minor = 0;
    if (!XRecordQueryVersion(control_, &major, &minor)) {
      cleanup();
      throw std::runtime_error("X11 RECORD extension is unavailable");
    }
    XRecordRange *range = XRecordAllocRange();
    if (!range) {
      cleanup();
      throw std::runtime_error("XRecordAllocRange failed");
    }
    range->device_events.first = KeyPress;
    range->device_events.last = KeyRelease;
    XRecordClientSpec clients = XRecordAllClients;
    context_ = XRecordCreateContext(control_, 0, &clients, 1, &range, 1);
    XFree(range);
    XSync(control_, False);
    if (!context_) {
      cleanup();
      throw std::runtime_error("XRecordCreateContext failed");
    }
    if (!XRecordEnableContextAsync(data_, context_, callback, reinterpret_cast<XPointer>(this))) {
      cleanup();
      throw std::runtime_error("XRecordEnableContextAsync failed");
    }
    enabled_ = true;
  }
  ~X11Backend() override {
    cleanup();
  }
  void pump() override {
    pollfd fd{ConnectionNumber(data_), POLLIN, 0};
    int ready = ::poll(&fd, 1, 0);
    if (ready > 0) {
      if (fd.revents & (POLLERR | POLLHUP | POLLNVAL))
        throw std::runtime_error("X11 keyboard connection closed");
      if (fd.revents & POLLIN)
        XRecordProcessReplies(data_);
    }
  }
  const char *name() const override {
    return "x11-xrecord";
  }
};
std::unique_ptr<Backend> start_backend(std::shared_ptr<Sink> sink) {
  return std::make_unique<X11Backend>(std::move(sink));
}
} // namespace niji::keyboard
