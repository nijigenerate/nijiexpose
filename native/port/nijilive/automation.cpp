// Direct translations of nijilive/core/automation/{package,sine,phys}.d.
#include "automation.hpp"
#include <core/timing.hpp>
namespace nijiexpose::automation {
namespace {
template<class T>void read(const json::Value& data,const char* name,T& value){if(auto field=json::find(data,name))json::deserialize(*field,value);}
void readVec(const json::Value& data,Vec2& value){if(data.is_null())return;if(!data.is_array())throw std::runtime_error("Expected a numeric vector array");auto& a=data.as_array();if(!a.empty())json::deserialize(a[0],value.x);if(a.size()>1)json::deserialize(a[1],value.y);}
}
float AutomationBinding::getAxisValue()const{switch(axis){case 0:return param->value.x;case 1:return param->value.y;default:return NAN;}}
void AutomationBinding::setAxisValue(float value){switch(axis){case 0:param->value.x=value;break;case 1:param->value.y=value;break;default:throw std::out_of_range("D automation axis");}}
void AutomationBinding::addAxisOffset(float value){param->pushIOffsetAxis(axis,value);}
json::Value AutomationBinding::serialize()const{return json::Object{{"param",param->name},{"axis",axis},{"range",json::Array{range.x,range.y}}};}
void AutomationBinding::deserializeFromJson(const json::Value& data){read(data,"param",paramId);read(data,"axis",axis);if(auto field=json::find(data,"axis"))readVec(*field,range);}// Source reads "axis", not "range"; retain the original rejection of numeric axis here.
void AutomationBinding::finalize(const std::shared_ptr<Puppet>& puppet){for(auto& p:puppet->parameters)if(p->name==paramId){param=p;return;}}
void Automation::reconstruct(const std::shared_ptr<Puppet>&){auto copy=bindings;for(auto& b:copy)b.reconstruct(parent.lock());}
void Automation::finalize(const std::shared_ptr<Puppet>& puppet){parent=puppet;for(auto& b:bindings)b.finalize(puppet);}
json::Value Automation::serialize()const{json::Array values;for(auto& b:bindings)values.push_back(b.serialize());json::Object data{{"type",typeId},{"name",name},{"bindings",std::move(values)}};serializeSelf(data);return data;}
void Automation::deserializeFromJson(const json::Value& data){read(data,"name",name);if(auto values=json::find(data,"bindings");values&&!values->is_null()){bindings.clear();for(auto& child:values->as_array()){AutomationBinding b;b.deserializeFromJson(child);bindings.push_back(std::move(b));}}deserializeSelf(data);}
void SineAutomation::onUpdate(){for(auto& b:bindings){double argument=nicxlive::core::currentTime()*speed+phase;double wave;switch(sineType){case SineType::Sin:wave=std::sin(argument);break;case SineType::Cos:wave=std::cos(argument);break;case SineType::Tan:wave=std::tan(argument);break;default:throw std::out_of_range("D sine type");}float value=remapRange(static_cast<float>((wave+1.0)/2.f),b.range);b.addAxisOffset(value);}}
void SineAutomation::serializeSelf(json::Object& data)const{data["speed"]=speed;data["sine_type"]=static_cast<int>(sineType);}
void SineAutomation::deserializeSelf(const json::Value& data){read(data,"speed",speed);if(auto field=json::find(data,"sine_type")){int value=static_cast<int>(sineType);json::deserialize(*field,value);sineType=static_cast<SineType>(value);}}
json::Value VerletNode::serialize()const{return json::Object{{"distance",distance},{"position",json::Array{position.x,position.y}},{"old_position",json::Array{oldPosition.x,oldPosition.y}}};}
void VerletNode::deserializeFromJson(const json::Value& data){read(data,"distance",distance);if(auto f=json::find(data,"position"))readVec(*f,position);if(auto f=json::find(data,"old_position"))readVec(*f,oldPosition);}
void PhysicsAutomation::simulate(size_t i,AutomationBinding&){auto& node=nodes.at(i);auto old=node.position;double dt=nicxlive::core::deltaTime();float dtSquared=static_cast<float>(dt*dt);float gx=0.f*dtSquared,gy=gravity*dtSquared;gx*=bounciness;gy*=bounciness;node.position={(node.position.x-node.oldPosition.x)+gx,(node.position.y-node.oldPosition.y)+gy};node.oldPosition=old;}
void PhysicsAutomation::constrain(){for(size_t i=0;i<nodes.size()-1;++i){auto& a=nodes.at(i);auto& b=nodes.at(i+1);if(i==0)a.position={bindings.at(i).getAxisValue(),0};float dx=a.position.x-b.position.x,dy=a.position.y-b.position.y;float dist=static_cast<float>(std::sqrt(static_cast<double>(dx*dx)+static_cast<double>(dy*dy))),diff=0;if(dist>0)diff=(b.distance-dist)/dist;float halfDiff=static_cast<float>(.5*diff);Vec2 trans{dx*halfDiff,dy*halfDiff};a.position.x+=trans.x;a.position.y+=trans.y;b.position.x-=trans.x;b.position.y-=trans.y;b.position.y=std::clamp(b.position.y,a.position.y,std::numeric_limits<float>::max());}}
void PhysicsAutomation::onUpdate(){if(bindings.size()>1){for(size_t i=0;i<bindings.size();++i)simulate(i,bindings[i]);for(size_t i=0;i<4+bindings.size()*2;++i)constrain();for(size_t i=0;i<nodes.size();++i){auto& node=nodes[i];auto& b=bindings.at(i);node.position.x=std::clamp(node.position.x,b.range.x,b.range.y);b.addAxisOffset(node.position.x);}}}
void PhysicsAutomation::serializeSelf(json::Object& data)const{json::Array values;for(auto& node:nodes)values.push_back(node.serialize());data["nodes"]=std::move(values);data["damping"]=damping;data["bounciness"]=bounciness;data["gravity"]=gravity;}
void PhysicsAutomation::deserializeSelf(const json::Value& data){if(auto values=json::find(data,"nodes");values&&!values->is_null()){nodes.clear();for(auto& child:values->as_array()){VerletNode node;node.deserializeFromJson(child);nodes.push_back(node);}}read(data,"damping",damping);read(data,"bounciness",bounciness);read(data,"gravity",gravity);}
bool inHasAutomationType(const std::string& id){return id=="sine"||id=="physics";}
std::shared_ptr<Automation> inInstantiateAutomation(const std::string& id,const std::shared_ptr<Puppet>& parent){if(id=="sine")return std::make_shared<SineAutomation>(parent);if(id=="physics")return std::make_shared<PhysicsAutomation>(parent);throw std::out_of_range("Unknown automation type");}
}
