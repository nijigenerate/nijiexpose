import reference_binding_impl,expression_context,reference_vspace_impl,reference_expression_impl;
import ft.adaptor,inmath;
import std.stdio;
class Source:Adaptor {
 override void start(){}override void stop(){}override void poll(){}
 override string[] getOptionNames(){return [];}
 override bool isRunning(){return true;}override bool isReceivingData(){return false;}
 override string getAdaptorName(){return "fixture";}
}
void print(string label,float value,bool valid=true){writefln("%s %s %d",label,*cast(uint*)&value,cast(int)valid);}
void main(){
 insScene.space=new VirtualSpace;auto zone=new VirtualSpaceZone("fixture");auto source=new Source;zone.sources=[source];source.getBlendshapes["x"]=0.25;insScene.space.addZone(zone);
 insInitExpressions();
 auto b=new TrackingBinding;b.param=new Parameter;b.param.min=vec2(-2,-3);b.param.max=vec2(2,3);b.param.defaults=vec2(0.1,0.2);
 auto ratio=new RatioTrackingBinding(b);b.delegated=ratio;b.update();print("empty",b.param.value.x);
 ratio.sourceName="x";ratio.outRangeToDefault();
 foreach(level;[0,1,10]){ratio.dampenLevel=level;foreach(i;0..4){source.getBlendshapes["x"]=i*0.25;b.update();print("ratio",b.param.value.x);}}
 ratio.inverse=true;b.update();print("inverse",b.param.value.x);
 insScene.space.update();b.param.value.x=0.123456;b.update();print("lost",b.param.value.x);
 auto event=new EventTrackingBinding(b);event.valueMap=[EventTrackingBinding.EventMap(SourceType.KeyPress,"",0.25),EventTrackingBinding.EventMap(SourceType.KeyPress,"a",0.9),EventTrackingBinding.EventMap(SourceType.KeyPress,"A",0.75)];
 b.delegated=event;b.update();print("default",b.param.value.x);keyDown=true;b.update();print("key",b.param.value.x);keyDown=false;event.dampenLevel=4;b.update();print("event-damp",b.param.value.x);
 auto expr=new ExpressionTrackingBinding(b);expr.expr.expression("0.75");float result;auto ok=expr.update(result);print("expression",result,ok);expr.expr.expression("0/0");ok=expr.update(result);print("nan",0,ok);
 auto comp=new CompoundTrackingBinding(b);
 comp.bindingMap=[CompoundTrackingBinding.BindingMap(comp,1,BindingType.EventBinding),CompoundTrackingBinding.BindingMap(comp,3,BindingType.EventBinding)];
 (cast(EventTrackingBinding)comp.bindingMap[0].delegated).valueMap=[EventTrackingBinding.EventMap(SourceType.KeyPress,"",0.25)];
 (cast(EventTrackingBinding)comp.bindingMap[1].delegated).valueMap=[EventTrackingBinding.EventMap(SourceType.KeyPress,"",0.75)];
 foreach(method;[CompoundTrackingBinding.Method.WeightedSum,CompoundTrackingBinding.Method.WeightedMul,CompoundTrackingBinding.Method.Ordered]){comp.method=method;ok=comp.update(result);print("compound",result,ok);}
 b.param.value.x=0;b.submit("a",0.5);b.submit("b",1.5);b.lateUpdate();print("plugin",b.param.value.x);b.lateUpdate();print("plugin-again",b.param.value.x);
 insCleanupExpressions();
}
