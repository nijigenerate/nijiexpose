#pragma once
#include "lua51.hpp"
#include <memory>
#include <string>
#include <vector>
#include <span>
namespace nijiexpose::plugins {
struct PluginInfo {std::string pluginId,pluginName,pluginVersion;};
class Plugin {
    std::string workingDirectory;
    PluginInfo info;
    lua_State* state;
    int environment=-1;
    bool encounteredError=false,enabled=false;
    static int requireCallback(lua_State*);
    int requireModule(const std::string& module);
public:
    Plugin(PluginInfo info,std::string directory,lua_State* state,bool enabled);
    bool hasEvent(const std::string& name);
    void callEvent(const std::string& name,std::span<const double> args={});
    void callEvent(const std::string& name,double arg){callEvent(name,std::span<const double>(&arg,1));}
    const char* getCName() const{return info.pluginName.c_str();}
    bool hasError() const{return encounteredError;}
    bool isEnabled() const{return enabled&&!encounteredError;}
    void isEnabled(bool value);
    const PluginInfo& getInfo() const{return info;}
};
extern thread_local std::vector<std::shared_ptr<Plugin>> insPlugins;
void insLuaInit();
void insLuaUnload();
void insSavePluginState();
void insEnumeratePlugins();
void insPluginBegnijiui();
void insPluginEndUI();
void insPluginRegisterAll(lua_State* state);
}
