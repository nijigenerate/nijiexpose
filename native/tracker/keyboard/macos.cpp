#include "internal.hpp"
#include <ApplicationServices/ApplicationServices.h>

namespace niji::keyboard {
class MacBackend final : public Backend {
  std::shared_ptr<Sink> sink_;
  CFMachPortRef tap_ = nullptr;
  CFRunLoopSourceRef source_ = nullptr;
  static CGEventRef callback(CGEventTapProxy, CGEventType type, CGEventRef event, void *pointer) {
    auto *self = static_cast<MacBackend *>(pointer);
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
      CGEventTapEnable(self->tap_, true);
      return event;
    }
    if (type == kCGEventKeyDown || type == kCGEventKeyUp) {
      UniChar text[8]{};
      UniCharCount count = 0;
      CGEventKeyboardGetUnicodeString(event, 8, &count, text);
      uint32_t cp = 0;
      if (count == 1)
        cp = text[0];
      else if (count == 2 && text[0] >= 0xd800 && text[0] <= 0xdbff && text[1] >= 0xdc00 &&
               text[1] <= 0xdfff)
        cp = 0x10000 + ((text[0] - 0xd800) << 10) + (text[1] - 0xdc00);
      if (cp >= 0xf700 && cp <= 0xf8ff)
        cp = 0; // macOS function-key characters are special keys.
      self->sink_->physical(
          static_cast<unsigned>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode)),
          type == kCGEventKeyDown, cp);
    }
    return event;
  }

public:
  explicit MacBackend(std::shared_ptr<Sink> sink) : sink_(std::move(sink)) {
    tap_ = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly,
                            CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp),
                            callback, this);
    if (!tap_)
      throw std::runtime_error(
          "CGEventTapCreate failed; grant Input Monitoring permission to the application");
    source_ = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap_, 0);
    if (!source_) {
      CFRelease(tap_);
      tap_ = nullptr;
      throw std::runtime_error("CFMachPortCreateRunLoopSource failed");
    }
    CFRunLoopAddSource(CFRunLoopGetCurrent(), source_, kCFRunLoopDefaultMode);
    CGEventTapEnable(tap_, true);
  }
  ~MacBackend() override {
    if (tap_)
      CGEventTapEnable(tap_, false);
    if (source_) {
      CFRunLoopRemoveSource(CFRunLoopGetCurrent(), source_, kCFRunLoopDefaultMode);
      CFRelease(source_);
    }
    if (tap_) {
      CFMachPortInvalidate(tap_);
      CFRelease(tap_);
    }
  }
  void pump() override {
    for (int i = 0; i < 256; i++)
      if (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true) != kCFRunLoopRunHandledSource)
        break;
  }
  const char *name() const override {
    return "macos-event-tap";
  }
};
std::unique_ptr<Backend> start_backend(std::shared_ptr<Sink> sink) {
  return std::make_unique<MacBackend>(std::move(sink));
}
} // namespace niji::keyboard
