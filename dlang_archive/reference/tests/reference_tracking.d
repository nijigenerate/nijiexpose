import nijiexpose.tracking.ext.exvmc;
import ft.adaptor;
import ft.adaptors.phizosc;
import ft.adaptors.llf;
import ft.adaptors.ifacialmocap, ft.adaptors.facemotion3d;
import ft.adaptors.openseeface;
import ft.adaptors.vtsproto;
import std.stdio, std.string, std.conv, std.algorithm;
import core.thread;
void main(string[] args) {
    Adaptor a;
    switch(args[1]) {
        case "vmc": a=new ExVMCAdaptor; break;
        case "llf": a=new LLFAdaptor; break;
        case "ifm": a=new IFMAdaptor; break;
        case "fm3d": a=new FM3DAdaptor; break;
        case "osf": a=new OSFAdaptor; break;
        case "vts": a=new VTSAdaptor; break;
        default: a=new PhizOSCAdaptor; break;
    }
    a.setOptions(["port":args[2],"address":"localhost","phoneIP":args[1]=="vts"?"255.255.255.255":"127.0.0.1","osf_bind_port":args[2],"osf_bind_ip":"localhost"]); a.start();
    writeln("READY"); stdout.flush();
    foreach(line;stdin.byLine) {
        auto command=line.idup.strip;
        if(command=="quit") break;
        Thread.sleep(50.msecs);
        foreach(i;0..command.to!int) a.poll();
        writefln("state %d %d %d %d",cast(int)a.isRunning,cast(int)a.isReceivingData,cast(int)neIsEventOn('a'),cast(int)neIsEventOn(0xE9));
        foreach(key;a.getBlendshapes.keys.sort) {
            float v=a.getBlendshapes[key]; writefln("blend %s %s",key,*cast(uint*)&v);
        }
        foreach(key;a.getBones.keys.sort) {
            auto b=a.getBones[key]; write("bone ",key);
            foreach(float v;[b.position.x,b.position.y,b.position.z,b.rotation.w,b.rotation.x,b.rotation.y,b.rotation.z]) write(" ",*cast(uint*)&v);
            writeln();
        }
        writeln("END"); stdout.flush();
    }
    a.stop();
    writeln("STOPPED"); stdout.flush();
    // Original Server finalization after explicit stop faults during runtime
    // shutdown on this Windows build. This oracle compares receive/poll/stop,
    // not D GC finalization; stop completed before terminating the test host.
    import core.stdc.stdlib : _Exit;
    _Exit(0);
}
