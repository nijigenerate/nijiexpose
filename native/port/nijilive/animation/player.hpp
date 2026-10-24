#pragma once
#include "animation.hpp"
namespace nijiexpose::animation {
class AnimationPlayback;
using AnimationPlaybackRef=std::shared_ptr<AnimationPlayback>;
class AnimationPlayer {
    friend class AnimationPlayback;
    std::shared_ptr<Puppet> puppet;
    std::vector<AnimationPlaybackRef> playingAnimations;
public:
    bool snapToFramerate=false;
    explicit AnimationPlayer(std::shared_ptr<Puppet> puppet):puppet(std::move(puppet)) {}
    void update(float delta);
    AnimationPlaybackRef createOrGet(const std::string& name);
    AnimationPlaybackRef play(const std::string& name);
    void prerenderAll();
    void stopAll(bool immediate=false);
    void destroyAll();
};
class AnimationPlayback {
    friend class AnimationPlayer;
    AnimationPlayer* player;
    std::shared_ptr<Animation> anim;
    bool valid=true;
    std::string _name;
    bool _playLeadOut=false,_paused=false,_playing=false,_looping=false,_stopping=false;
    float _time=0,_strength=1,_speed=1;
    int _looped=0;
    void update(float delta);
public:
    AnimationPlayback(AnimationPlayer* player,std::shared_ptr<Animation> anim,std::string name):player(player),anim(std::move(anim)),_name(std::move(name)) {}
    const std::string& name() const { return _name; }
    bool eof() const { return frame()>=anim->length; }
    bool isValid() const { return valid; }
    bool playing() const { return _playing; }
    bool stopping() const { return _stopping; }
    bool paused() const { return _paused; }
    bool looping() const { return _looping; }
    bool looping(bool value) { return _looping=value; }
    int looped() const { return _looped; }
    float speed() const { return _speed; }
    float speed(bool value) { _speed=static_cast<float>(std::clamp(int(value),1,10)); return value; }
    float strength() const { return _strength; }
    float strength(float value) { _strength=std::clamp(value,0.f,1.f); return value; }
    int frame() const { return static_cast<int>(std::round(_time/anim->timestep)); }
    float hframe() const { return _time/anim->timestep; }
    int loopPointEnd() const { return hasLeadOut()?anim->leadOut:anim->length; }
    int loopPointBegin() const { return hasLeadIn()?anim->leadIn:0; }
    bool hasLeadIn() const { return anim->leadIn>0 && anim->leadIn+1<anim->length; }
    bool hasLeadOut() const { return anim->leadOut>0 && anim->leadOut+1<anim->length; }
    bool isPlayingLeadOut() const { return ((_playing&&!_looping)||_stopping)&&_playLeadOut&&frame()<anim->length; }
    bool isRunning() const { return _playing||isPlayingLeadOut(); }
    int fps() const { return static_cast<int>(1000.0/(anim->timestep*1000.0)); }
    int seconds() const { return static_cast<int>(_time); }
    int miliseconds() const { return static_cast<int>((_time-static_cast<float>(seconds()))*1000); }
    int frames() const { return anim->length; }
    Animation* animation() const { return anim.get(); }
    ptrdiff_t playbackId() const;
    void destroy();
    void play(bool loop=false,bool playLeadOut=true);
    void pause() { _paused=true; }
    void stop(bool immediate=false);
    void seek(int frame);
    void render();
};
}
