#include "nijiexpose/plugins/package.hpp"
#include <iostream>
using namespace nijiexpose::plugins;
int main(int argc,char** argv){if(argc!=2)return 1;auto s=luaL_newstate();lua_pushvalue(s,LUA_GLOBALSINDEX);lua_setfield(s,LUA_GLOBALSINDEX,"_G");insPluginRegisterAll(s);Plugin p({"fixture","Fixture","1"},argv[1],s,true);std::cout<<std::boolalpha;
std::cout<<"init "<<p.isEnabled()<<' '<<p.hasEvent("onUpdate")<<' '<<p.hasEvent("absent")<<'\n';p.callEvent("onUpdate",0.25);std::cout<<"update ok\n";
try{p.callEvent("onFail");}catch(const std::exception& ex){std::cout<<"failure "<<ex.what()<<'\n';}std::cout<<"disabled "<<p.hasError()<<' '<<p.isEnabled()<<'\n';p.isEnabled(true);p.callEvent("onReinit");std::cout<<"reinit "<<p.hasError()<<' '<<p.isEnabled()<<'\n';p.isEnabled(false);p.callEvent("onFail");std::cout<<"off "<<p.hasError()<<'\n';p.isEnabled(true);try{p.callEvent("onMissing");}catch(const std::exception& ex){std::cout<<"missing "<<ex.what()<<'\n';}p.isEnabled(true);try{p.callEvent("onInvalid");}catch(const std::exception& ex){std::cout<<"invalid "<<ex.what()<<'\n';}lua_close(s);
}
