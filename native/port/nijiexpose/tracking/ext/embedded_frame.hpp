#pragma once
#include "embedded.hpp"
#include "embedded.h"
namespace nijiexpose::tracking {
inline void publishEmbeddedFrame(const NtFrame& frame){
    auto& feed=embeddedFeed();
    feed.received=std::chrono::steady_clock::now();
    feed.active=frame.face_present!=0;
    feed.blends.clear();feed.bones.clear();
    if(!feed.active)return;
    for(int i=0;i<frame.blend_count;++i)feed.blends[frame.blends[i].name]=frame.blends[i].value;
    // Preserve nijiexpose's existing head coordinate convention without OSC serialization.
    const auto& q=frame.quaternion;
    feed.bones["Head"]={{frame.position[0],frame.position[1],frame.position[2]},{q[3],-q[2],q[0],-q[1]}};
}
}
