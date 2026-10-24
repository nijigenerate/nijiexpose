#pragma once
#include <cstddef>
// Lua 5.1 ABI used by the original Windows lumars dependency.
extern "C" {
struct lua_State;
using lua_CFunction=int(*)(lua_State*);
lua_State* luaL_newstate();
void lua_close(lua_State*);
int lua_gettop(lua_State*);
void lua_settop(lua_State*,int);
int lua_type(lua_State*,int);
double lua_tonumber(lua_State*,int);
const char* lua_tolstring(lua_State*,int,std::size_t*);
void lua_pushnumber(lua_State*,double);
void lua_pushcclosure(lua_State*,lua_CFunction,int);
void lua_pushstring(lua_State*,const char*);
void lua_setfield(lua_State*,int,const char*);
void lua_getfield(lua_State*,int,const char*);
int luaL_loadstring(lua_State*,const char*);
int lua_pcall(lua_State*,int,int,int);
int lua_error(lua_State*);
int luaL_error(lua_State*,const char*,...);
void lua_pushvalue(lua_State*,int);
void lua_pushnil(lua_State*);
void lua_pushboolean(lua_State*,int);
void lua_pushlightuserdata(lua_State*,void*);
int lua_toboolean(lua_State*,int);
void* lua_touserdata(lua_State*,int);
const void* lua_topointer(lua_State*,int);
void lua_createtable(lua_State*,int,int);
void lua_rawgeti(lua_State*,int,int);
void lua_rawseti(lua_State*,int,int);
void lua_rawset(lua_State*,int);
int lua_setmetatable(lua_State*,int);
int lua_setfenv(lua_State*,int);
void lua_remove(lua_State*,int);
int luaL_ref(lua_State*,int);
void luaL_unref(lua_State*,int,int);
int luaL_callmeta(lua_State*,int,const char*);
const char* luaL_optlstring(lua_State*,int,const char*,std::size_t*);
}
inline constexpr int LUA_GLOBALSINDEX=-10002;
inline constexpr int LUA_REGISTRYINDEX=-10000;
inline void lua_pop(lua_State* state,int count) {lua_settop(state,-count-1);}
