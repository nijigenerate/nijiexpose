#pragma once
#include "nijiui/input.hpp"
#include <cstdint>
namespace nijiui {
struct ImageRect { float x=0,y=0,width=1,height=1; };
void uiImImage(uint32_t id,vec2 size,ImageRect textureCutout={},vec4 tintcolor={1,1,1,1},vec4 bordercolor={0,0,0,0});
}
