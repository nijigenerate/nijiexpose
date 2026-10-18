#include "runtime/clock.hpp"

#include <timing.hpp>
#include <chrono>
#include <utility>

namespace nijiexpose {

RuntimeClock::RuntimeClock(std::function<double()> source) {
    if (!source) {
        const auto epoch = std::chrono::steady_clock::now();
        source = [epoch] {
            return std::chrono::duration<double>(std::chrono::steady_clock::now() - epoch).count();
        };
    }
    // Renderer initialization is deferred until the P1 backend exists.
    nicxlive::core::inSetTimingFunc(std::move(source));
    nicxlive::core::inUpdate(); // Establish the first-frame baseline.
}

RuntimeClock::~RuntimeClock() {
    nicxlive::core::inSetTimingFunc({});
}

double RuntimeClock::tick() {
    nicxlive::core::inUpdate();
    return nicxlive::core::deltaTime();
}

} // namespace nijiexpose
