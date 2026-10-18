#include "runtime/clock.hpp"

#include <cmath>
#include <cstdio>

int main() {
    double seconds = 100.0;
    nijiexpose::RuntimeClock clock([&seconds] { return seconds; });
    seconds += 0.025;
    if (std::abs(clock.tick() - 0.025) > 1e-9) {
        std::fputs("nicxlive did not advance by the supplied frame interval\n", stderr);
        return 1;
    }
    if (std::abs(clock.tick()) > 1e-9) {
        std::fputs("Repeated tick at the same time advanced nicxlive\n", stderr);
        return 1;
    }
    return 0;
}
