module reference_plugin;
import reference_plugin_impl,reference_plugin_base,lumars,bindbc.lua,std.stdio,std.file,std.path;
void main(string[] args){
    stderr.writeln("state");auto state=new LuaState(luaL_newstate());state.copy(LUA_GLOBALSINDEX);state.setGlobal("_G");insRegisterBaseAPI(state);stderr.writeln("create");
    auto p=new Plugin(PluginInfo("fixture","Fixture","1"),args[1],state,LuaTable.init,true);
    stderr.writeln("created");writeln("init ",p.isEnabled," ",p.hasEvent("onUpdate")," ",p.hasEvent("absent"));stdout.flush();
    p.callEvent("onUpdate",0.25);writeln("update ok");stdout.flush();stderr.writeln("fail event");
    try{p.callEvent("onFail");}catch(Exception ex){writeln("failure ",ex.msg);}
    writeln("disabled ",p.hasError," ",p.isEnabled);
    p.isEnabled=true;p.callEvent("onReinit");writeln("reinit ",p.hasError," ",p.isEnabled);
    p.isEnabled=false;p.callEvent("onFail");writeln("off ",p.hasError);
    p.isEnabled=true;try{p.callEvent("onMissing");}catch(Exception ex){writeln("missing ",ex.msg);}
    p.isEnabled=true;try{p.callEvent("onInvalid");}catch(Exception ex){writeln("invalid ",ex.msg);}
}
