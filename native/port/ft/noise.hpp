#pragma once
#include <cstdint>
namespace nijiexpose::math {
// The 2D OpenSimplex path used by the original expression API.
void osseed(uint64_t seed);
double osnoise2(double x,double y);
}
