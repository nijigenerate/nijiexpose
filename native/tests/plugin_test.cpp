#include "nijiexpose/plugins/package.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace nijiexpose::plugins;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
    check(argc==2,"Expected isolated directory");auto dir=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(dir/"sub");
    {std::ofstream f(dir/"sub"/"module.lua");f<<"value=42";}
    {std::ofstream f(dir/"init.lua");f<<R"(count=0
function onInit() count=count+1 end
function onUpdate(dt)
 assert(count==1)
 assert(dt==0.25)
 assert(require('sub.module').value==42)
 assert(require('sub/module')==require('sub.module'))
 assert(type(tonumber('12'))=='text')
 assert(type(tonumber('12',16))=='text')
 assert(tostring('ignored', 16)=='16')
 assert(scene~=nil and ui~=nil)
 assert(ui.button('inactive')==false)
 assert(math==nil and os==nil)
end
function onFail() assert(false,'fixture failure') end
function onReinit() assert(count==2) end
function onMissing() require('missing') end
function onInvalid() require('../module') end
)";}
    auto s=luaL_newstate();lua_pushvalue(s,LUA_GLOBALSINDEX);lua_setfield(s,LUA_GLOBALSINDEX,"_G");insPluginRegisterAll(s);
    Plugin p({"fixture","Fixture","1"},dir.string(),s,true);check(p.isEnabled()&&p.hasEvent("onUpdate")&&!p.hasEvent("absent"),"Event discovery failed");p.callEvent("onUpdate",0.25);
    try{p.callEvent("onFail");check(false,"Error did not propagate");}catch(const std::runtime_error& ex){check(std::string(ex.what()).find("fixture failure")!=std::string::npos,"Wrong Lua error");}
    check(p.hasError()&&!p.isEnabled(),"Error did not disable plugin");p.isEnabled(true);check(!p.hasError(),"Reenable retained error");p.callEvent("onReinit");p.isEnabled(false);p.callEvent("onFail");check(!p.hasError(),"Disabled event ran");
    p.isEnabled(true);try{p.callEvent("onMissing");check(false,"Missing module accepted");}catch(const std::runtime_error& ex){check(std::string(ex.what()).find("Module missing not found")!=std::string::npos,"Missing module message");}
    p.isEnabled(true);try{p.callEvent("onInvalid");check(false,"Invalid module accepted");}catch(const std::runtime_error& ex){check(std::string(ex.what()).find("invalid module specifier")!=std::string::npos,"Invalid module message");}
    lua_close(s);std::cout<<"Plugin environments, require cache, events, error disable and reenable passed\n";return 0;
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}}
