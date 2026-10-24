#pragma once
#include "json.hpp"
#include "expr.hpp"
#include <core/puppet.hpp>
#include <core/param/parameter.hpp>
namespace nijiexpose::tracking {
using Parameter=nicxlive::core::param::Parameter;
using Puppet=nicxlive::core::Puppet;
using Vec2=nicxlive::core::math::Vec2;
enum class BindingType {RatioBinding,ExpressionBinding,EventBinding,CompoundBinding,External};
enum class SourceType {Blendshape,BonePosX,BonePosY,BonePosZ,BoneRotRoll,BoneRotPitch,BoneRotYaw,KeyPress};
class ITrackingBinding {
public:
    virtual ~ITrackingBinding()=default;
    virtual void serializeSelf(json::Object& serializer) const=0;
    virtual void deserializeFromJson(const json::Value& data)=0;
    virtual void outRangeToDefault()=0;
    virtual bool update(float& result)=0;
};
class TrackingBinding {
protected:
    uint32_t paramUUID=0;
    float sum=0,weights=0;
public:
    std::shared_ptr<ITrackingBinding> delegated;
    std::string name;
    BindingType type_=BindingType::RatioBinding;
    std::shared_ptr<Parameter> param;
    std::map<std::string,float> pluginWeights;
    int axis=0;
    BindingType type() const {return type_;}
    void type(BindingType value);
    json::Value serialize() const;
    void deserializeFromJson(const json::Value& data);
    void outRangeToDefault() {if(delegated)delegated->outRangeToDefault();}
    bool finalize(const std::shared_ptr<Puppet>& puppet) {param=puppet->findParameter(paramUUID);return param!=nullptr;}
    void update();
    void submit(const std::string& plugin,float value);
    void lateUpdate();
};
class RatioTrackingBinding final:public ITrackingBinding {
public:
    TrackingBinding* binding;
    explicit RatioTrackingBinding(TrackingBinding* binding):binding(binding) {}
    float inVal=0,outVal=0;
    Vec2 inRange{0,1},outRange{0,1};
    SourceType sourceType=SourceType::Blendshape;
    std::string sourceName,sourceDisplayName;
    bool inverse=false;
    int dampenLevel=0;
    void serializeSelf(json::Object&) const override;
    void deserializeFromJson(const json::Value&) override;
    void outRangeToDefault() override;
    bool update(float& result) override;
    void createSourceDisplayName();
};
class ExpressionTrackingBinding final:public ITrackingBinding {
    TrackingBinding* binding_;
public:
    explicit ExpressionTrackingBinding(TrackingBinding* binding);
    float inVal=0,outVal=0;
    std::unique_ptr<Expression> expr;
    TrackingBinding* binding() const {return binding_;}
    int dampenLevel=0;
    void serializeSelf(json::Object&) const override;
    void deserializeFromJson(const json::Value&) override;
    void outRangeToDefault() override {}
    bool update(float& result) override;
};
class EventTrackingBinding final:public ITrackingBinding {
public:
    explicit EventTrackingBinding(TrackingBinding* binding):binding(binding) {}
    TrackingBinding* binding;
    float outVal=0;
    int dampenLevel=0;
    struct EventMap {SourceType type=SourceType::Blendshape;std::string id;float value=std::numeric_limits<float>::quiet_NaN();};
    std::vector<EventMap> valueMap;
    void serializeSelf(json::Object&) const override;
    void deserializeFromJson(const json::Value&) override;
    void outRangeToDefault() override {}
    bool update(float& result) override;
};
class CompoundTrackingBinding final:public ITrackingBinding {
    TrackingBinding* binding_;
public:
    explicit CompoundTrackingBinding(TrackingBinding* binding):binding_(binding) {}
    float outVal=0;
    TrackingBinding* binding() const {return binding_;}
    enum class Method {WeightedSum,WeightedMul,Ordered};
    Method method=Method::WeightedSum;
    int dampenLevel=0;
    struct BindingMap {
        float weight=1;
        BindingType type_=BindingType::RatioBinding;
        std::shared_ptr<ITrackingBinding> delegated;
        CompoundTrackingBinding* compound=nullptr;
        BindingMap()=default;
        BindingMap(CompoundTrackingBinding* binding,float weight,BindingType type):weight(weight),type_(type),delegated(binding->createBinding(type)),compound(binding) {}
        BindingType type() const {return type_;}
        void type(BindingType value) {type_=value;delegated=compound->createBinding(type_);}
    };
    std::vector<BindingMap> bindingMap;
    std::shared_ptr<ITrackingBinding> createBinding(BindingType type);
    void serializeSelf(json::Object&) const override;
    void deserializeFromJson(const json::Value&) override;
    void outRangeToDefault() override {}
    bool update(float& result) override;
};
}
