#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace niji::keyboard {
struct Event {
  bool pressed;
  std::string text;
  uint32_t codepoint;
};
struct Sink;
class Backend;
// Captures ordinary characters globally. poll() pumps the OS event queue and
// returns events; call it frequently on the thread which constructed Capture.
class Capture {
  std::shared_ptr<Sink> sink_;
  std::unique_ptr<Backend> backend_;

public:
  Capture();
  ~Capture();
  Capture(const Capture &) = delete;
  Capture &operator=(const Capture &) = delete;
  std::vector<Event> poll();
  const char *backend_name() const;
};
} // namespace niji::keyboard
