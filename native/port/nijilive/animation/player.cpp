// Direct port of nijilive/core/animation/player.d.
#include "player.hpp"
namespace nijiexpose::animation {
void AnimationPlayer::update(float delta) { for(auto& anim:playingAnimations) if(anim->valid) anim->update(delta); }
AnimationPlaybackRef AnimationPlayer::createOrGet(const std::string& name) {
    for(auto& anim:playingAnimations) if(anim->_name==name) return anim;
    if(auto it=puppet->animations.find(name);it!=puppet->animations.end()) {
        auto animation=std::dynamic_pointer_cast<Animation>(it->second);
        if(!animation) throw std::logic_error("Animation was not loaded with the native animation implementation");
        playingAnimations.push_back(std::make_shared<AnimationPlayback>(this,animation,name));
        return playingAnimations.back();
    }
    return nullptr;
}
AnimationPlaybackRef AnimationPlayer::play(const std::string& name) { auto anim=createOrGet(name); if(anim) anim->play(); return anim; }
void AnimationPlayer::prerenderAll() { for(auto anim:playingAnimations) anim->render(); }
void AnimationPlayer::stopAll(bool immediate) { for(auto anim:playingAnimations) anim->stop(immediate); }
void AnimationPlayer::destroyAll() { for(auto anim:playingAnimations) anim->valid=false; playingAnimations.clear(); }
void AnimationPlayback::update(float delta) {
    if(!valid||!isRunning()) return;
    if(_paused) { render(); return; }
    _time+=delta;
    if(!isPlayingLeadOut()&&looping()&&frame()>=loopPointEnd()) { _time=static_cast<float>(loopPointBegin())*anim->timestep; ++_looped; }
    if(frame()+1>=frames()) _time=static_cast<float>(frames()-1)*anim->timestep;
    render();
    if(!_looping&&isPlayingLeadOut()) {
        if(frame()+1>=anim->length) { _playing=false; _playLeadOut=false; _stopping=false; _time=0; _looped=0; }
    }
}
ptrdiff_t AnimationPlayback::playbackId() const {
    ptrdiff_t idx=-1;
    for(size_t i=0;i<player->playingAnimations.size();++i) if(player->playingAnimations[i]->_name==_name) idx=static_cast<ptrdiff_t>(i);
    return idx;
}
void AnimationPlayback::destroy() { valid=false; if(playbackId()>-1) player->playingAnimations.erase(player->playingAnimations.begin()+playbackId()); }
void AnimationPlayback::play(bool loop,bool playLeadOut) {
    if(_paused) _paused=false;
    else { _looped=0; _time=0; _stopping=false; _playing=true; _looping=loop; _playLeadOut=playLeadOut; }
}
void AnimationPlayback::stop(bool immediate) {
    if(_stopping) return;
    bool shouldStopImmediate=immediate||frame()==0||_paused||!hasLeadOut();
    _stopping=!shouldStopImmediate; _looping=false; _paused=false; _playing=false; _playLeadOut=!shouldStopImmediate;
    if(shouldStopImmediate) { _time=0; _looped=0; }
}
void AnimationPlayback::seek(int frame) { float frameTime=static_cast<float>(std::clamp(frame,0,frames())); _time=frameTime*anim->timestep; _looped=0; }
void AnimationPlayback::render() {
    float realStrength=std::clamp(_strength,0.f,1.f);
    for(auto lane:anim->lanes) lane.paramRef->targetParam->pushIOffsetAxis(lane.paramRef->targetAxis,lane.get(hframe(),player->snapToFramerate)*realStrength,lane.mergeMode);
}
}
