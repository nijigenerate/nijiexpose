#include "nijiexpose/tracking/package.hpp"
#include "nijiexpose/tracking/context.hpp"
#include <core/timing.hpp>
#include <imgui.h>
#include <iostream>
#include <bit>
using namespace nijiexpose;
using namespace nijiexpose::tracking;
namespace nijiexpose::tracking {std::shared_ptr<VirtualSpace> insCurrentTrackingSpace(){static auto space=std::make_shared<VirtualSpace>();return space;}}
struct Source:ft::Adaptor {
    void start()override{}void stop()override{}void poll()override{}
    std::vector<std::string> getOptionNames()override{return {};}
    bool isRunning()override{return true;}bool isReceivingData()override{return false;}
    std::string getAdaptorName()override{return "fixture";}
};
void print(const char* label,float value,bool valid=true){std::cout<<label<<' '<<std::bit_cast<uint32_t>(value)<<' '<<valid<<'\n';}
int main(){
    ImGui::CreateContext();nicxlive::core::inSetTimingFunc([]{return 0.016;});nicxlive::core::inUpdate();
    auto space=insCurrentTrackingSpace();auto zone=std::make_shared<VirtualSpaceZone>("fixture");auto source=std::make_shared<Source>();zone->sources={source};source->getBlendshapes()["x"]=0.25;space->addZone(zone);
    insInitExpressions();TrackingBinding b;b.param=std::make_shared<Parameter>();b.param->min={-2,-3};b.param->max={2,3};b.param->defaults={0.1f,0.2f};
    auto ratio=std::make_shared<RatioTrackingBinding>(&b);b.delegated=ratio;b.update();print("empty",b.param->value.x);
    ratio->sourceName="x";ratio->outRangeToDefault();
    for(int level:{0,1,10}){ratio->dampenLevel=level;for(int i=0;i<4;++i){source->getBlendshapes()["x"]=static_cast<float>(i*0.25);b.update();print("ratio",b.param->value.x);}}
    ratio->inverse=true;b.update();print("inverse",b.param->value.x);
    space->update();b.param->value.x=0.123456f;b.update();print("lost",b.param->value.x);
    auto event=std::make_shared<EventTrackingBinding>(&b);event->valueMap={{SourceType::KeyPress,"",0.25f},{SourceType::KeyPress,"a",0.9f},{SourceType::KeyPress,"A",0.75f}};
    b.delegated=event;b.update();print("default",b.param->value.x);ImGui::GetIO().KeysData[ImGuiKey_A-ImGuiKey_NamedKey_BEGIN].Down=true;b.update();print("key",b.param->value.x);ImGui::GetIO().KeysData[ImGuiKey_A-ImGuiKey_NamedKey_BEGIN].Down=false;event->dampenLevel=4;b.update();print("event-damp",b.param->value.x);
    ExpressionTrackingBinding expr(&b);expr.expr->expression("0.75");float result;bool ok=expr.update(result);print("expression",result,ok);expr.expr->expression("0/0");ok=expr.update(result);print("nan",0,ok);
    CompoundTrackingBinding comp(&b);comp.bindingMap={{&comp,1,BindingType::EventBinding},{&comp,3,BindingType::EventBinding}};
    std::dynamic_pointer_cast<EventTrackingBinding>(comp.bindingMap[0].delegated)->valueMap={{SourceType::KeyPress,"",0.25f}};
    std::dynamic_pointer_cast<EventTrackingBinding>(comp.bindingMap[1].delegated)->valueMap={{SourceType::KeyPress,"",0.75f}};
    for(auto method:{CompoundTrackingBinding::Method::WeightedSum,CompoundTrackingBinding::Method::WeightedMul,CompoundTrackingBinding::Method::Ordered}){comp.method=method;ok=comp.update(result);print("compound",result,ok);}
    b.param->value.x=0;b.submit("a",0.5f);b.submit("b",1.5f);b.lateUpdate();print("plugin",b.param->value.x);b.lateUpdate();print("plugin-again",b.param->value.x);
    insCleanupExpressions();ImGui::DestroyContext();
}
