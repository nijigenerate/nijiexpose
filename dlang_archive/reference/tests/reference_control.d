import reference_control_impl,expression_context,reference_vspace_impl;
import ft.adaptor;
import nijilive,nijilive.core.animation,nijilive.core.animation.player;
import std.stdio;
class Source:Adaptor{
 override void start(){}override void stop(){}override void poll(){}
 override string[] getOptionNames(){return [];}override bool isRunning(){return true;}override bool isReceivingData(){return true;}override string getAdaptorName(){return "fixture";}
}
void main(){
 insScene.space=new VirtualSpace;auto z=new VirtualSpaceZone("fixture");auto s=new Source;z.sources=[s];insScene.space.addZone(z);
 foreach(mode;0..3){
  auto puppet=new Puppet;Animation a;a.timestep=.016f;a.length=100;a.leadIn=0;a.leadOut=90;puppet.getAnimations["test"]=a;auto player=new AnimationPlayer(puppet);
  auto c=new AnimationControl;c.name="test";c.finalize(player);
  if(mode==0){c.type=TriggerType.Event;c.playEvent=cast(TriggerEvent)(TriggerEvent.Load|TriggerEvent.Idle);c.fullStopEvent=TriggerEvent.Sleep;}
  else{c.type=TriggerType.Tracking;c.sourceName="x";if(mode==2){c.defaultThresholds=false;c.playThresholdValue=.5;c.stopThresholdValue=0;c.fullStopThresholdValue=-.5;c.useHoldDelay=true;c.playThresholdHoldDelay=32;c.stopThresholdHoldDelay=32;c.fullStopThresholdHoldDelay=32;}}
  float[] inputs=[-2,-2,-1,0,0,.5,1,1,1,.5,0,0,-1,-1,1,1,1,0,0,-1];
  foreach(i,v;inputs){s.getBlendshapes["x"]=v;if(mode==0&&i==10)c.sleep();if(mode==0&&i==14)c.awake();c.update();player.update(.016f);float value=c.inVal;writefln("%s %s %s %s %s %d %d %d %s %s %s %s",mode,i,cast(int)c.event,c.load_wait,*cast(uint*)&value,cast(int)c.anim.playing,cast(int)c.anim.stopping,cast(int)c.anim.looping,c.anim.frame,c.playThresholdHoldTime,c.stopThresholdHoldTime,c.fullStopThresholdHoldTime);}
 }
}
