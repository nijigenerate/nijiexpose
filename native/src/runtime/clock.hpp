#pragma once

#include <functional>

namespace nijiexpose {

// nicxlive timing is process-global. Only one clock may be active at a time.
class RuntimeClock {
public:
    explicit RuntimeClock(std::function<double()> source = {});
    ~RuntimeClock();
    RuntimeClock(const RuntimeClock&) = delete;
    RuntimeClock& operator=(const RuntimeClock&) = delete;
    double tick();
};

} // namespace nijiexpose
