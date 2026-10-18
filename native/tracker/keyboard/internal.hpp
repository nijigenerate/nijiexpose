#pragma once
#include "keyboard.hpp"
#include <map>
#include <stdexcept>
#include <utility>

namespace niji::keyboard {
struct Sink {
  std::vector<Event> events;
  std::map<unsigned, Event> held;
  bool overflow = false;
  void physical(unsigned key, bool down, uint32_t unicode);
};
class Backend {
public:
  virtual ~Backend() = default;
  virtual void pump() = 0;
  virtual const char *name() const = 0;
};
std::unique_ptr<Backend> start_backend(std::shared_ptr<Sink> sink);
} // namespace niji::keyboard
