module nijilive.reference_animation;
import nijilive;
import nijilive.core.animation;
import nijilive.core.animation.player;
import std.stdio;
void main() {
    AnimationLane lane;
    lane.frames=[Keyframe(0,-.7f,.5f),Keyframe(3,.3f,.15f),Keyframe(9,1.8f,.9f),Keyframe(15,-.2f,1.4f)];
    foreach(mode;0..5) foreach(snap;[false,true]) {
        lane.interpolation=cast(InterpolateMode)mode;
        foreach(i;-8..80) {
            float result=lane.get(i*.25f,snap);
            writefln("lane:%s:%s:%s:%08x",mode,cast(int)snap,i,*cast(uint*)&result);
        }
    }
    auto puppet=new Puppet();
    Animation a; a.timestep=.125f; a.length=12; a.leadIn=2; a.leadOut=8;
    puppet.getAnimations()["test"]=a;
    auto player=new AnimationPlayer(puppet);
    auto anim=player.createOrGet("test");
    void state(int n) { writefln("state:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s",n,anim.frame,cast(int)anim.playing,cast(int)anim.paused,cast(int)anim.stopping,cast(int)anim.looping,anim.looped,anim.seconds,anim.miliseconds,cast(int)anim.isValid); }
    foreach(loop;[false,true]) {
        anim.play(loop); state(0);
        foreach(i;1..22) { player.update(.125f); state(i); }
        anim.pause(); player.update(.25f); state(30);
        anim.play(false); player.update(.125f); state(31);
        anim.stop(); player.update(.125f); state(32);
        foreach(i;33..40) { player.update(.125f); state(i); }
        anim.stop(true); state(40);
        anim.seek(20); state(41);
    }
    anim.destroy(); state(42);
    writeln("missing:",cast(int)(player.createOrGet("missing") is null));
}
