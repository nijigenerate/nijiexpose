#include "internal.hpp"
#define WIN32_LEAN_AND_MEAN
#include <array>
#include <windows.h>

namespace niji::keyboard {
class WindowsBackend final : public Backend {
  std::shared_ptr<Sink> sink_;
  HHOOK hook_ = nullptr;
  std::array<BYTE, 256> state_{};
  static thread_local WindowsBackend *instance_;
  static LRESULT CALLBACK callback(int code, WPARAM kind, LPARAM parameter) {
    auto *self = instance_;
    if (code == HC_ACTION && self) {
      auto &key = *reinterpret_cast<KBDLLHOOKSTRUCT *>(parameter);
      bool down = kind == WM_KEYDOWN || kind == WM_SYSKEYDOWN;
      bool up = kind == WM_KEYUP || kind == WM_SYSKEYUP;
      if ((down || up) && key.vkCode < 256) {
        self->state_[key.vkCode] = BYTE((self->state_[key.vkCode] & 1) | (down ? 0x80 : 0));
        if (down && key.vkCode == VK_CAPITAL)
          self->state_[VK_CAPITAL] ^= 1;
        self->state_[VK_SHIFT] = BYTE(self->state_[VK_LSHIFT] | self->state_[VK_RSHIFT]);
        self->state_[VK_CONTROL] = BYTE(self->state_[VK_LCONTROL] | self->state_[VK_RCONTROL]);
        self->state_[VK_MENU] = BYTE(self->state_[VK_LMENU] | self->state_[VK_RMENU]);
        WCHAR text[8]{};
        uint32_t cp = 0;
        DWORD thread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
        int count = down ? ToUnicodeEx(key.vkCode, key.scanCode, self->state_.data(), text, 8, 4,
                                       GetKeyboardLayout(thread))
                         : 0;
        if (count == 1)
          cp = text[0];
        else if (count == 2 && text[0] >= 0xd800 && text[0] <= 0xdbff && text[1] >= 0xdc00 &&
                 text[1] <= 0xdfff)
          cp = 0x10000 + ((text[0] - 0xd800) << 10) + (text[1] - 0xdc00);
        self->sink_->physical(static_cast<unsigned>(key.scanCode) |
                                  (static_cast<unsigned>(key.flags & LLKHF_EXTENDED) << 16),
                              down, cp);
      }
    }
    return CallNextHookEx(nullptr, code, kind, parameter);
  }

public:
  explicit WindowsBackend(std::shared_ptr<Sink> sink) : sink_(std::move(sink)) {
    if (instance_)
      throw std::runtime_error("Only one keyboard capture per thread is supported");
    GetKeyboardState(state_.data());
    instance_ = this;
    hook_ = SetWindowsHookExW(WH_KEYBOARD_LL, callback, GetModuleHandleW(nullptr), 0);
    if (!hook_) {
      instance_ = nullptr;
      throw std::runtime_error("SetWindowsHookExW failed: " + std::to_string(GetLastError()));
    }
  }
  ~WindowsBackend() override {
    if (hook_)
      UnhookWindowsHookEx(hook_);
    instance_ = nullptr;
  }
  // The host SDL event loop dispatches Windows messages on this thread.
  void pump() override {}
  const char *name() const override {
    return "windows-low-level-hook";
  }
};
thread_local WindowsBackend *WindowsBackend::instance_ = nullptr;
std::unique_ptr<Backend> start_backend(std::shared_ptr<Sink> sink) {
  return std::make_unique<WindowsBackend>(std::move(sink));
}
} // namespace niji::keyboard
