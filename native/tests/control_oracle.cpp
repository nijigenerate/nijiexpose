#include "animation/package.hpp"
#include "tracking/context.hpp"
#include <iostream>
#include <bit>
using namespace nijiexpose;
namespace nijiexpose::tracking {std::shared_ptr<VirtualSpace> insCurrentTrackingSpace(){static auto space=std::make_shared<VirtualSpace>();return space;}}
struct Source:ft::Adaptor{
 void start()override{}void stop()override{}void poll()override{}
 std::vector<std::string> getOptionNames()override{return {};}bool isRunning()override{return true;}bool isReceivingData()override{return true;}std::string getAdaptorName()override{return "fixture";}
};
int main(){
 using namespace animation;
 nicxlive::core::inSetTimingFunc([]{return 0.016;});nicxlive::core::inUpdate();
 auto space=tracking::insCurrentTrackingSpace();auto z=std::make_shared<tracking::VirtualSpaceZone>("fixture");auto s=std::make_shared<Source>();z->sources={s};space->addZone(z);
 for(int mode=0;mode<3;++mode){
  auto puppet=std::make_shared<Puppet>();auto a=std::make_shared<Animation>();a->timestep=.016f;a->length=100;a->leadIn=0;a->leadOut=90;puppet->animations["test"]=a;AnimationPlayer player(puppet);
  AnimationControl c;c.name="test";c.finalize(player);
  if(mode==0){c.type=TriggerType::Event;c.playEvent=int(TriggerEvent::Load)|int(TriggerEvent::Idle);c.fullStopEvent=int(TriggerEvent::Sleep);}
  else{c.type=TriggerType::Tracking;c.sourceName="x";if(mode==2){c.defaultThresholds=false;c.playThresholdValue=.5;c.stopThresholdValue=0;c.fullStopThresholdValue=-.5;c.useHoldDelay=true;c.playThresholdHoldDelay=32;c.stopThresholdHoldDelay=32;c.fullStopThresholdHoldDelay=32;}}
  const float inputs[]={-2,-2,-1,0,0,.5,1,1,1,.5,0,0,-1,-1,1,1,1,0,0,-1};
  for(int i=0;i<20;++i){s->getBlendshapes()["x"]=inputs[i];if(mode==0&&i==10)c.sleep();if(mode==0&&i==14)c.awake();c.update();player.update(.016f);std::cout<<mode<<' '<<i<<' '<<int(c.event)<<' '<<c.load_wait<<' '<<std::bit_cast<uint32_t>(c.inVal)<<' '<<c.anim->playing()<<' '<<c.anim->stopping()<<' '<<c.anim->looping()<<' '<<c.anim->frame()<<' '<<c.playThresholdHoldTime<<' '<<c.stopThresholdHoldTime<<' '<<c.fullStopThresholdHoldTime<<'\n';}
 }
}
