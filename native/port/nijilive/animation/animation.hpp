#pragma once
#include "nijiexpose/json.hpp"
#include <core/puppet.hpp>
namespace nijiexpose::animation {
using nicxlive::core::Puppet;
using nicxlive::core::Parameter;
using nicxlive::core::param::ParamMergeMode;
enum class InterpolateMode { Nearest,Linear,Stepped,Cubic,Bezier,COUNT };
struct Keyframe {
    int frame=0;
    float value=std::numeric_limits<float>::quiet_NaN();
    float tension=.5f;
    json::Value serializeToJson() const;
    void deserializeFromJson(const json::Value&);
};
struct AnimationParameterRef { std::shared_ptr<Parameter> targetParam; int targetAxis=0; };
struct AnimationLane {
    uint32_t refuuid=0;
    std::shared_ptr<AnimationParameterRef> paramRef;
    std::vector<Keyframe> frames;
    InterpolateMode interpolation=InterpolateMode::Nearest;
    ParamMergeMode mergeMode=ParamMergeMode::Forced;
    float get(float frame,bool snapSubframes=false) const;
    void reconstruct(const std::shared_ptr<Puppet>&) {} // Empty in the D source.
    void finalize(const std::shared_ptr<Puppet>& puppet);
    void updateFrames();
    json::Value serializeToJson() const;
    void deserializeFromJson(const json::Value&);
};
struct Animation final:nicxlive::core::Animation {
    float timestep=.0166f;
    bool additive=false;
    float animationWeight=std::numeric_limits<float>::quiet_NaN();
    std::vector<AnimationLane> lanes;
    int length=0,leadIn=-1,leadOut=-1;
    void reconstruct(const std::shared_ptr<Puppet>&) override;
    void finalize(const std::shared_ptr<Puppet>&) override;
    json::Value serializeToJson() const;
    void deserializeFromJson(const json::Value&);
};
}
