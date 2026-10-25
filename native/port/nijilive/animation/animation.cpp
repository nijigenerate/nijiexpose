// Port of nijilive/core/animation/animation.d and inmath/interpolate.d formulae.
#include "animation.hpp"
#include <array>
#include <cmath>
namespace nijiexpose::animation {
namespace {
constexpr std::array modes{"Nearest","Linear","Stepped","Cubic","Bezier","COUNT"};
constexpr std::array mergeNames{"Additive","Weighted","Multiplicative","Forced","Passthrough"};
template<class T> void read(const json::Value& object,const char* key,T& out) {
    if(auto value=json::find(object,key)) json::deserialize(*value,out);
}
template<class E,size_t N> void readEnum(const json::Value& data,const char* key,E& out,const std::array<const char*,N>& names,bool flexible=false) {
    auto value=json::find(data,key); if(!value) return;
    if(value->is_string()) {
        std::string text(value->as_string());
        for(size_t i=0;i<N;++i) {
            std::string name(names[i]);
            if(text==name) { out=static_cast<E>(i); return; }
            if(flexible) { for(auto& c:name) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c))); if(text==name) { out=static_cast<E>(i); return; } }
        }
    }
    if(flexible && value->is_number()) { int n=0; json::deserialize(*value,n); out=static_cast<E>(n); }
}
float lerp(float a,float b,float t) { return a*(1-t)+b*t; }
// D's float^^3 uses std.math.pow with a real intermediate (double on Windows).
// float^^2 is the library's special case and multiplies in float.
float cube(float t) { double v=t; return static_cast<float>(v*(v*v)); }
float cubic(float p0,float p1,float p2,float p3,float t) {
    float a=static_cast<float>(-.5*p0+1.5*p1-1.5*p2+.5*p3);
    float b=static_cast<float>(p0-2.5*p1+2*p2-.5*p3);
    float c=static_cast<float>(-.5*p0+.5*p2),d=p1;
    return a*cube(t)+b*(t*t)+c*t+d;
}
float hermite(float x,float tx,float y,float ty,float t) {
    float h1=2*cube(t)-3*(t*t)+1,h2=-2*cube(t)+3*(t*t);
    float h3=cube(t)-2*(t*t)+t,h4=cube(t)-(t*t);
    return h1*x+h3*tx+h2*y+h4*ty;
}
}
json::Value Keyframe::serializeToJson() const { return json::Object{{"frame",frame},{"value",json::valueFrom(value)},{"tension",json::valueFrom(tension)}}; }
void Keyframe::deserializeFromJson(const json::Value& data) { read(data,"frame",frame); read(data,"value",value); read(data,"tension",tension); }
float AnimationLane::get(float frame,bool snap) const {
    if(!frames.empty()) {
        if(snap) frame=std::floor(frame);
        if(frames.size()==1) return frames[0].value;
        for(size_t i=0;i<frames.size();++i) {
            if(frames[i].frame<frame) continue;
            if(i==0) return frames[0].value;
            float tonext=static_cast<float>(frames[i].frame)-frame;
            float ilen=static_cast<float>(frames[i].frame)-static_cast<float>(frames[i-1].frame);
            float t=1-(tonext/ilen),tension=frames[i].tension;
            switch(interpolation) {
            case InterpolateMode::Nearest: return t>.5 ? frames[i].value : frames[i-1].value;
            case InterpolateMode::Stepped: return frames[i-1].value;
            case InterpolateMode::Linear: return lerp(frames[i-1].value,frames[i].value,t);
            case InterpolateMode::Cubic: {
                float prev=frames[i<2?0:i-2].value,curr=frames[i-1].value;
                float next1=frames[std::min(i,frames.size()-1)].value,next2=frames[std::min(i+1,frames.size()-1)].value;
                return cubic(prev,curr,next1,next2,t);
            }
            case InterpolateMode::Bezier: return lerp(frames[i-1].value,frames[i].value,std::clamp(hermite(0,2*tension,1,2*tension,t),0.f,1.f));
            default: throw std::logic_error("Invalid animation interpolation");
            }
        }
        return frames.back().value;
    }
    return 0;
}
void AnimationLane::finalize(const std::shared_ptr<Puppet>& puppet) { if(paramRef) paramRef->targetParam=puppet->findParameter(refuuid); }
void AnimationLane::updateFrames() { std::stable_sort(frames.begin(),frames.end(),[](const auto& a,const auto& b){return a.frame<b.frame;}); }
json::Value AnimationLane::serializeToJson() const {
    json::Object out{{"interpolation",modes.at(static_cast<size_t>(interpolation))}};
    if(paramRef) { out["uuid"]=paramRef->targetParam->uuid; out["target"]=paramRef->targetAxis; }
    out["keyframes"]=json::valueFrom(frames); out["merge_mode"]=mergeNames.at(static_cast<size_t>(mergeMode));
    return out;
}
void AnimationLane::deserializeFromJson(const json::Value& data) {
    readEnum(data,"interpolation",interpolation,modes); read(data,"uuid",refuuid);
    paramRef=std::make_shared<AnimationParameterRef>(); read(data,"target",paramRef->targetAxis);
    read(data,"keyframes",frames); readEnum(data,"merge_mode",mergeMode,mergeNames,true);
}
void Animation::reconstruct(const std::shared_ptr<Puppet>& puppet) { auto copy=lanes; for(auto& lane:copy) lane.reconstruct(puppet); }
void Animation::finalize(const std::shared_ptr<Puppet>& puppet) { for(auto& lane:lanes) lane.finalize(puppet); }
json::Value Animation::serializeToJson() const {
    json::Object out{{"timestep",json::valueFrom(timestep)},{"additive",additive},{"length",length},{"leadIn",leadIn},{"leadOut",leadOut},{"animationWeight",json::valueFrom(animationWeight)}};
    json::Array array;
    for(const auto& lane:lanes) if(lane.paramRef->targetParam) array.push_back(lane.serializeToJson());
    out["lanes"]=std::move(array); return out;
}
void Animation::deserializeFromJson(const json::Value& data) {
    read(data,"timestep",timestep); read(data,"additive",additive); read(data,"animationWeight",animationWeight);
    read(data,"length",length); read(data,"leadIn",leadIn); read(data,"leadOut",leadOut); read(data,"lanes",lanes);
}
}
