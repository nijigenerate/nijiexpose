#include "internal.hpp"

namespace niji::keyboard {
static std::string utf8(uint32_t c) {
  std::string out;
  if (c < 0x80)
    out += char(c);
  else if (c < 0x800) {
    out += char(0xc0 | (c >> 6));
    out += char(0x80 | (c & 63));
  } else if (c < 0x10000) {
    out += char(0xe0 | (c >> 12));
    out += char(0x80 | ((c >> 6) & 63));
    out += char(0x80 | (c & 63));
  } else {
    out += char(0xf0 | (c >> 18));
    out += char(0x80 | ((c >> 12) & 63));
    out += char(0x80 | ((c >> 6) & 63));
    out += char(0x80 | (c & 63));
  }
  return out;
}
void Sink::physical(unsigned key, bool down, uint32_t unicode) {
  Event e{};
  auto existing = held.find(key);
  if (down) {
    if (existing != held.end())
      e = existing->second;
    else {
      // Special/control keys were not sent by the Python implementation either.
      if (unicode <= 32 || (unicode >= 0x7f && unicode < 0xa0) || unicode > 0x10ffff ||
          (unicode >= 0xd800 && unicode <= 0xdfff))
        return;
      e = {true, utf8(unicode), unicode};
      held.emplace(key, e);
    }
    e.pressed = true;
  } else {
    if (existing == held.end())
      return;
    e = existing->second;
    e.pressed = false;
    held.erase(existing);
  }
  if (events.size() >= 4096) {
    overflow = true;
    return;
  }
  events.push_back(std::move(e));
}
Capture::Capture() : sink_(std::make_shared<Sink>()), backend_(start_backend(sink_)) {}
Capture::~Capture() = default;
std::vector<Event> Capture::poll() {
  backend_->pump();
  if (sink_->overflow)
    throw std::runtime_error("Keyboard event queue overflow");
  std::vector<Event> out;
  out.swap(sink_->events);
  return out;
}
const char *Capture::backend_name() const {
  return backend_->name();
}
} // namespace niji::keyboard
