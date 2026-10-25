#include "nijilive/animation/player.hpp"
#include <bit>
#include <cstdio>
using namespace nijiexpose::animation;
int main() {
    AnimationLane lane;
    lane.frames={{0,-.7f,.5f},{3,.3f,.15f},{9,1.8f,.9f},{15,-.2f,1.4f}};
    for(int mode=0;mode<5;++mode) for(bool snap:{false,true}) {
        lane.interpolation=static_cast<InterpolateMode>(mode);
        for(int i=-8;i<80;++i) std::printf("lane:%d:%d:%d:%08x\n",mode,int(snap),i,std::bit_cast<uint32_t>(lane.get(i*.25f,snap)));
    }
    auto puppet=std::make_shared<Puppet>();
    auto a=std::make_shared<Animation>(); a->timestep=.125f; a->length=12; a->leadIn=2; a->leadOut=8;
    puppet->animations["test"]=a;
    AnimationPlayer player(puppet);
    auto anim=player.createOrGet("test");
    auto state=[&](int n) { std::printf("state:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n",n,anim->frame(),int(anim->playing()),int(anim->paused()),int(anim->stopping()),int(anim->looping()),anim->looped(),anim->seconds(),anim->miliseconds(),int(anim->isValid())); };
    for(bool loop:{false,true}) {
        anim->play(loop); state(0);
        for(int i=1;i<22;++i) { player.update(.125f); state(i); }
        anim->pause(); player.update(.25f); state(30);
        anim->play(false); player.update(.125f); state(31);
        anim->stop(); player.update(.125f); state(32);
        for(int i=33;i<40;++i) { player.update(.125f); state(i); }
        anim->stop(true); state(40);
        anim->seek(20); state(41);
    }
    anim->destroy(); state(42);
    std::printf("missing:%d\n",int(player.createOrGet("missing")==nullptr));
}
