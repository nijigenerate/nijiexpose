#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace niji {
// Constant memory, including when a camera runs for hours.
struct TimingSamples {
  std::uint64_t count = 0;
  double sum = 0, squared_sum = 0, maximum = 0;
  void add(double milliseconds) {
    ++count;
    sum += milliseconds;
    squared_sum += milliseconds * milliseconds;
    maximum = std::max(maximum, milliseconds);
  }
  double mean() const { return count ? sum / static_cast<double>(count) : 0; }
  double deviation() const {
    return count ? std::sqrt(std::max(0.0, squared_sum / static_cast<double>(count) - mean()*mean())) : 0;
  }
};
struct CaptureTiming {
  TimingSamples intervals, reads;
};
} // namespace niji
