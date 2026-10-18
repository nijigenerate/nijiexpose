#pragma once
#include "nijiexpose/json.hpp"
#include <core/puppet.hpp>
namespace nijiexpose::automation {
using Parameter=nicxlive::core::param::Parameter;
using Puppet=nicxlive::core::Puppet;
using Vec2=nicxlive::core::math::Vec2;
struct AutomationBinding {
    std::string paramId;
    std::shared_ptr<Parameter> param;
    int axis=0;
    Vec2 range{NAN,NAN};
    float getAxisValue() const;
    void setAxisValue(float value);
    void addAxisOffset(float value);
    json::Value serialize() const;
    void deserializeFromJson(const json::Value& data);
    void reconstruct(const std::shared_ptr<Puppet>&) {}
    void finalize(const std::shared_ptr<Puppet>& puppet);
};
class Automation : public nicxlive::core::Automation {
    std::weak_ptr<Puppet> parent;
protected:
    std::vector<AutomationBinding> bindings;
    static float remapRange(float value,Vec2 range){return range.x+value*(range.y-range.x);}
    virtual void onUpdate() {}
    virtual void serializeSelf(json::Object&) const {}
    virtual void deserializeSelf(const json::Value&) {}
public:
    std::string name,typeId;
    bool enabled=true;
    explicit Automation(const std::shared_ptr<Puppet>& parent):parent(parent){}
    virtual void bind(AutomationBinding binding){bindings.push_back(std::move(binding));}
    void reconstruct(const std::shared_ptr<Puppet>& puppet) override;
    void finalize(const std::shared_ptr<Puppet>& puppet) final;
    void update() final{if(enabled)onUpdate();}
    json::Value serialize() const;
    void deserializeFromJson(const json::Value& data);
};
enum class SineType {Sin,Cos,Tan};
class SineAutomation : public Automation {
    void onUpdate() override;
    void serializeSelf(json::Object&) const override;
    void deserializeSelf(const json::Value&) override;
public:
    float speed=1,phase=0;
    SineType sineType=SineType::Sin;
    explicit SineAutomation(const std::shared_ptr<Puppet>& parent):Automation(parent){typeId="sine";}
};
struct VerletNode {
    float distance=1;
    Vec2 position{NAN,NAN},oldPosition{NAN,NAN};
    VerletNode()=default;
    explicit VerletNode(Vec2 position):position(position),oldPosition(position){}
    json::Value serialize() const;
    void deserializeFromJson(const json::Value& data);
};
class PhysicsAutomation : public Automation {
    void simulate(size_t i,AutomationBinding& binding);
    void constrain();
    void onUpdate() override;
    void serializeSelf(json::Object&) const override;
    void deserializeSelf(const json::Value&) override;
public:
    std::vector<VerletNode> nodes;
    float damping=.05f,bounciness=1,gravity=20;
    void bind(AutomationBinding binding) override{Automation::bind(std::move(binding));nodes.emplace_back(Vec2{0,1});}
    explicit PhysicsAutomation(const std::shared_ptr<Puppet>& parent):Automation(parent){typeId="physics";}
};
std::shared_ptr<Automation> inInstantiateAutomation(const std::string& id,const std::shared_ptr<Puppet>& parent);
bool inHasAutomationType(const std::string& id);
}
