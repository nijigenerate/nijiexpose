// Original Windows plugin lifecycle, local environments, require, and API registration.
#include "package.hpp"
#include "nijiexpose/log.hpp"
#include "nijiui/core/settings.hpp"
#include "nijiui/core/path.hpp"
#include "nijiui/widgets/input.hpp"
#include "nijiui/widgets/dialog.hpp"
#include "i18n/tr.hpp"
#include <filesystem>
#include <fstream>
#include <format>
#include <cstdio>
namespace nijiexpose::plugins {
thread_local std::vector<std::shared_ptr<Plugin>> insPlugins;
namespace {
thread_local lua_State* state=nullptr;
thread_local bool uiBegun=false;
const char* kind(int type){static const char* names[]={"nil","boolean","lightUserdata","number","text","table","func","userdata","thread"};return type>=0&&type<9?names[type]:"none";}
std::string text(lua_State* s,int index){size_t length=0;auto p=lua_tolstring(s,index,&length);return p?std::string(p,length):std::string{};}
std::string readFile(const std::filesystem::path& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Cannot read "+path.string());return {std::istreambuf_iterator<char>(f),{}};}
void checked(lua_State* s,int status){if(status){auto error=text(s,-1);lua_pop(s,1);throw std::runtime_error(error);}}
void run(lua_State* s,const std::string& script,int env=-1){checked(s,luaL_loadstring(s,script.c_str()));if(env!=-1){lua_rawgeti(s,LUA_REGISTRYINDEX,env);if(!lua_setfenv(s,-2))throw std::runtime_error("Failed to set function environment");}checked(s,lua_pcall(s,0,env==-1?-1:0,0));}
void push(lua_State* s,int ref){lua_rawgeti(s,LUA_REGISTRYINDEX,ref);}
void field(lua_State* s,const char* key,const std::string& value){lua_pushstring(s,value.c_str());lua_setfield(s,-2,key);}
std::string idxToString(lua_State* s,int i){switch(lua_type(s,i)){case 0:return "nil";case 1:return lua_toboolean(s,i)?"true":"false";case 3:{char buffer[128];std::snprintf(buffer,sizeof(buffer),"%.6g",lua_tonumber(s,i));return buffer;}case 4:return text(s,i);default:return std::format("{}: {:x}",kind(lua_type(s,i)),reinterpret_cast<uintptr_t>(lua_topointer(s,i)));}}
int print(lua_State* s){int level=static_cast<int>(lua_tonumber(s,LUA_GLOBALSINDEX-1));std::string out;int n=lua_gettop(s);for(int i=1;i<=n;++i){if(i>1)out+='\t';out+=idxToString(s,i);}lua_pop(s,n);if(level==2)insLogWarn("%s",out);else if(level==3)insLogErr("%s",out);else insLogInfo("%s",out);return 0;}
int toString(lua_State* s){if(luaL_callmeta(s,1,"__tostring"))return 1;if(lua_gettop(s)>0){auto out=idxToString(s,lua_gettop(s));lua_pushstring(s,out.c_str());}else lua_pushnil(s);return 1;}
int assertion(lua_State* s){if(lua_gettop(s)<1)return luaL_error(s,"bad argument #1 to 'assert' (value expected)");if(!lua_toboolean(s,1))return luaL_error(s,"%s",luaL_optlstring(s,2,"assertation failed!",nullptr));return 0;}
int type(lua_State* s){if(lua_gettop(s)!=1)return luaL_error(s,"A D function threw an exception: Expected exactly 1 args, but was given %d.",lua_gettop(s));lua_pushstring(s,kind(lua_type(s,1)));return 1;}
int uiCallback(lua_State* s){
    int id=static_cast<int>(lua_tonumber(s,LUA_GLOBALSINDEX-1)),count=id<2?1:2;
    if(lua_gettop(s)!=count)return luaL_error(s,"A D function threw an exception: Expected exactly %d args, but was given %d.",count,lua_gettop(s));
    for(int i=1;i<=count;++i)if(lua_type(s,i)!=4)return luaL_error(s,"A D function threw an exception: Expected value at stack index %d to be of type text but it is %s",i,kind(lua_type(s,i)));
    auto first=text(s,1),second=text(s,2);
    if(id==0){lua_pushboolean(s,uiBegun&&nijiui::uiImButton(first.c_str()));return 1;}
    if(id==1){if(uiBegun)nijiui::uiImLabel(first);return 0;}
    if(id==2){bool edited=nijiui::uiImInputText(first,second);lua_createtable(s,2,0);lua_pushboolean(s,edited);lua_rawseti(s,-2,1);lua_pushstring(s,second.c_str());lua_rawseti(s,-2,2);return 1;}
    nijiui::uiImDialog(first.c_str(),second,id==3?nijiui::DialogLevel::Error:id==4?nijiui::DialogLevel::Info:nijiui::DialogLevel::Warning);return 0;
}
int inheritedEnvironment(lua_State* s,const std::string& name,int parent){
    lua_createtable(s,0,0);int ref=luaL_ref(s,LUA_REGISTRYINDEX);push(s,ref);lua_setfield(s,LUA_GLOBALSINDEX,name.c_str());
    push(s,ref);lua_createtable(s,0,1);push(s,parent);lua_setfield(s,-2,"__index");lua_setmetatable(s,-2);lua_pop(s,1);return ref;
}
std::string replaceFormat(std::string fmt,const std::string& arg){auto at=fmt.find("%s");if(at!=std::string::npos)fmt.replace(at,2,arg);return fmt;}
}
void insPluginRegisterAll(lua_State* s){
    const char* names[]={"print","info","warn","err"};for(int i=0;i<4;++i){lua_pushnumber(s,i);lua_pushcclosure(s,print,1);lua_setfield(s,LUA_GLOBALSINDEX,names[i]);}
    for(auto name:{"tostring","tonumber"}){lua_pushcclosure(s,toString,0);lua_setfield(s,LUA_GLOBALSINDEX,name);} // D registers l_tostring for both.
    lua_pushcclosure(s,assertion,0);lua_setfield(s,LUA_GLOBALSINDEX,"assert");lua_pushcclosure(s,type,0);lua_setfield(s,LUA_GLOBALSINDEX,"type");
    lua_createtable(s,0,0);lua_pushvalue(s,-1);lua_setfield(s,LUA_GLOBALSINDEX,"scene");
    lua_createtable(s,0,6);const char* uiNames[]={"button","label","textbox","error","info","warn"};for(int i=0;i<6;++i){lua_pushnumber(s,i);lua_pushcclosure(s,uiCallback,1);lua_setfield(s,-2,uiNames[i]);}lua_pushvalue(s,-1);lua_setfield(s,LUA_GLOBALSINDEX,"ui");
}
Plugin::Plugin(PluginInfo value,std::string directory,lua_State* lua,bool enable):workingDirectory(std::move(directory)),info(std::move(value)),state(lua),enabled(enable){
    lua_createtable(state,0,3);environment=luaL_ref(state,LUA_REGISTRYINDEX);push(state,environment);
    lua_createtable(state,0,3);field(state,"id",info.pluginId);field(state,"name",info.pluginName);field(state,"version",info.pluginVersion);lua_setfield(state,-2,"PLUGIN_INFO");
    lua_createtable(state,0,0);lua_setfield(state,-2,"LOADED");lua_pushlightuserdata(state,this);lua_pushcclosure(state,requireCallback,1);lua_setfield(state,-2,"require");
    lua_pushvalue(state,-1);lua_setfield(state,LUA_GLOBALSINDEX,info.pluginId.c_str());lua_createtable(state,0,1);lua_getfield(state,LUA_GLOBALSINDEX,"_G");lua_setfield(state,-2,"__index");lua_pushvalue(state,-1);lua_setmetatable(state,-3);lua_remove(state,-2); // D createEnvironment leaves meta.push() on the stack.
    run(state,readFile(std::filesystem::u8path(workingDirectory)/"init.lua"),environment);insLogInfo(i18n::_("Loaded plugin %s %s..."),info.pluginName,info.pluginVersion);
    if(enabled)try{if(hasEvent("onInit"))callEvent("onInit");}catch(const std::exception& ex){insLogErr(i18n::_("%s (plugin): %s"),info.pluginName,ex.what());}
}
bool Plugin::hasEvent(const std::string& name){push(state,environment);lua_getfield(state,-1,name.c_str());bool result=lua_type(state,-1)==6;lua_pop(state,2);return result;}
void Plugin::callEvent(const std::string& name,std::span<const double> args){
    if(!isEnabled())return;int top=lua_gettop(state);push(state,environment);lua_getfield(state,-1,name.c_str());lua_remove(state,-2);push(state,environment);if(!lua_setfenv(state,-2))throw std::runtime_error("Failed to set function environment");
    for(double arg:args)lua_pushnumber(state,arg);int status=lua_pcall(state,static_cast<int>(args.size()),0,0);if(status){encounteredError=true;auto error=text(state,-1);lua_settop(state,top);throw std::runtime_error(error);}
}
void Plugin::isEnabled(bool value){enabled=value;encounteredError=false;if(enabled&&hasEvent("onInit"))callEvent("onInit");}
int Plugin::requireCallback(lua_State* s){
    bool failed=false;int ref=-1;
    {try{if(lua_gettop(s)!=1||lua_type(s,1)!=4)throw std::runtime_error("Expected one module name");auto self=static_cast<Plugin*>(lua_touserdata(s,LUA_GLOBALSINDEX-1));ref=self->requireModule(text(s,1));}catch(const std::exception& ex){lua_pushstring(s,ex.what());failed=true;}}
    if(failed)return lua_error(s);push(s,ref);return 1;
}
int Plugin::requireModule(const std::string& module){
    auto canonical=module;for(char& c:canonical)if(c=='/'||c=='\\')c='.';
    if(canonical.empty()||canonical.front()=='.'||canonical.back()=='.'||canonical.find("..")!=std::string::npos){encounteredError=true;throw std::runtime_error(replaceFormat(i18n::_("Attempted to load invalid module specifier %s"),module));}
    push(state,environment);lua_getfield(state,-1,"LOADED");lua_getfield(state,-1,canonical.c_str());if(lua_type(state,-1)==5){int ref=luaL_ref(state,LUA_REGISTRYINDEX);lua_pop(state,2);return ref;}lua_pop(state,3);
    auto file=module;for(char& c:file)if(c=='.')c='/';auto relative=std::filesystem::u8path(file);relative.replace_extension("lua");auto base=std::filesystem::u8path(workingDirectory),path=base/relative;
    if(!std::filesystem::exists(path))throw std::runtime_error(replaceFormat(i18n::_("Module %s not found"),canonical));
    std::filesystem::path segments;for(const auto& segment:relative){segments/=segment;if(std::filesystem::is_symlink(base/segments)){encounteredError=true;throw std::runtime_error(i18n::_("Attempted to require a symlink, this is not allowed."));}}
    int ref=inheritedEnvironment(state,canonical,environment);run(state,readFile(path),ref);push(state,environment);lua_getfield(state,-1,"LOADED");push(state,ref);lua_setfield(state,-2,canonical.c_str());lua_pop(state,2);return ref;
}
void insSavePluginState(){json::Object states;for(auto& p:insPlugins)states[p->getInfo().pluginId]=json::Object{{"isEnabled",p->isEnabled()}};nijiui::inSettingsSetJson("pluginStates",std::move(states));}
void insEnumeratePlugins(){
    insPlugins.clear();auto directory=nijiui::inGetAppCustomPath("plugins");insLogInfo("Scanning plugins at %s...",directory);auto states=nijiui::inSettingsGet<json::Value>("pluginStates");
    for(auto& entry:std::filesystem::directory_iterator(std::filesystem::u8path(directory))){auto init=entry.path()/"init.lua",infoFile=entry.path()/"info.lua";auto u8=entry.path().filename().u8string();std::string name(u8.begin(),u8.end());
        if(std::filesystem::exists(init)&&std::filesystem::exists(infoFile)){
            run(state,"return "+readFile(infoFile));
            try{if(lua_type(state,-1)!=5)throw std::runtime_error("Expected plugin information table");PluginInfo info;for(auto pair:{std::pair{"pluginId",&info.pluginId},{"pluginName",&info.pluginName},{"pluginVersion",&info.pluginVersion}}){lua_getfield(state,-1,pair.first);*pair.second=text(state,-1);lua_pop(state,1);}lua_pop(state,2);info.pluginId=name;
                bool enable=true;if(auto saved=json::find(states,name))if(auto v=json::find(*saved,"isEnabled"))json::deserialize(*v,enable);
                auto dir=entry.path().u8string();insPlugins.push_back(std::make_shared<Plugin>(info,std::string(dir.begin(),dir.end()),state,enable));
            }catch(const std::exception& ex){insLogErr("Plugin %s failed to initialize, %s.",name,ex.what());}
        }else insLogWarn("Invalid plugin %s...",name);
    }insSavePluginState();
}
void insLuaInit(){insLogInfo("Lua support initialized. (Statically linked)");state=luaL_newstate();if(!state)throw std::bad_alloc();lua_pushvalue(state,LUA_GLOBALSINDEX);lua_setfield(state,LUA_GLOBALSINDEX,"_G");insPluginRegisterAll(state);insEnumeratePlugins();}
void insLuaUnload(){lua_close(state);state=nullptr;}
void insPluginBegnijiui(){uiBegun=true;}
void insPluginEndUI(){uiBegun=false;}
}
