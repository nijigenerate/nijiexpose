// Direct translation of nijiui/core/app.d (BSD-2-Clause).
#include "nijiui/core/app.hpp"
namespace nijiui {
namespace { InApplication APP_INFO; } // D explicitly uses __gshared for this field.
void inSetApplication(InApplication app) { APP_INFO = app; }
InApplication& inGetApplication() { return APP_INFO; }
}
