#pragma once
#include "vspace.hpp"
namespace nijiexpose::tracking {
// Supplied by the scene module; test hosts supply their fixture scene here.
std::shared_ptr<VirtualSpace> insCurrentTrackingSpace();
}
