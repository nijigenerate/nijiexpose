#pragma once
namespace nijiexpose::dcompat {
// D Error is separate from Exception; do not translate an array bounds
// failure into an exception caught by source catch(Exception) handlers.
struct RangeError {};
}
