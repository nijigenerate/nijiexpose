import reference_vspace_impl;
import ft.adaptor, ft.data;
import inmath.linalg;
import std.stdio;
class Source: Adaptor {
    bool active; int polls;
    override void start() {} override void stop() {} override void poll() {polls++;}
    override string[] getOptionNames(){return [];}
    override bool isRunning(){return true;} override bool isReceivingData(){return active;}
    override string getAdaptorName(){return "Fixture";}
}
void main() {
    auto a=new Source; auto b=new Source; auto c=new Source;
    auto space=new VirtualSpace;
    auto za=new VirtualSpaceZone("a"); auto zb=new VirtualSpaceZone("b"); auto zc=new VirtualSpaceZone("c");
    za.sources=[a,b,null]; zb.sources=[b]; zc.sources=[c];
    space.addZone(za); space.addZone(zb); space.addZone(zc);
    foreach(step;0..7) {
        if(step==1){b.active=true;c.active=true;}
        if(step==3){c.active=false;}
        if(step==5){space.removeZone(zb);}
        space.update();
        writefln("focus %s %d %d %d %d",space.currentZone.name,cast(int)space.hasAnyFocus,a.polls,b.polls,c.polls);
    }
    a.getBlendshapes["x"]=0.25; b.getBlendshapes["x"]=0.75;
    writefln("blend %s %s",za.getBlendshapeFor("x"),za.getBlendshapeFor("absent"));
    foreach(i;0..20) {
        a.getBones["head"]=Bone(vec3(1,2,3),quat.eulerRotation(i*0.05,i*0.02,i*-0.01));
        b.getBones["head"]=Bone(vec3(-1,4,7),quat.eulerRotation(i*-0.03,i*0.07,i*0.04));
        auto bone=za.getBoneFor("head");
        write("bone");
        foreach(float v;[bone.position.x,bone.position.y,bone.position.z,bone.rotation.w,bone.rotation.x,bone.rotation.y,bone.rotation.z]) write(" ",*cast(uint*)&v);
        writeln();
    }
}
