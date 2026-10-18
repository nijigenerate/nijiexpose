// source/nijiexpose/tracking/expr.d, using the same Windows Lua 5.1 runtime.
#include "expr.hpp"
#include "context.hpp"
#include "nijiexpose/plugins/lua51.hpp"
#include "nijiexpose/log.hpp"
#include "ft/noise.hpp"
#include "dcompat/trigonometry.hpp"
#include "i18n/tr.hpp"
#include <core/timing.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <random>
namespace nijiexpose::tracking {
namespace {
thread_local lua_State* state=nullptr;
const char* kind(int t) {static constexpr const char* names[]={"nil","boolean","lightUserdata","number","text","table","function","userdata","thread"};return t>=0&&t<9?names[t]:"none";}
std::string text(lua_State* s,int index) {size_t size=0;const char* p=lua_tolstring(s,index,&size);return p?std::string(p,size):std::string{};}
struct Function {const char* name;int count;};
constexpr Function functions[]={
    {"BLEND",1},{"BONE_X",1},{"BONE_Y",1},{"BONE_Z",1},{"ROLL",1},{"PITCH",1},{"YAW",1},{"time",0},
    {"sin",1},{"cos",1},{"tan",1},{"sinh",1},{"cosh",1},{"tanh",1},{"psin",1},{"pcos",1},{"ptan",1},
    {"usin",1},{"ucos",1},{"utan",1},{"abs",1},{"sqrt",1},{"floor",1},{"ceil",1},{"round",1},
    {"min",2},{"max",2},{"clamp",3},{"lerp",3},{"cubic",5},{"atan2",2},{"degrees",1},{"radians",1},{"simplex",1},{"usimplex",1},{"sign",1}};
int callback(lua_State* s) {
    int id=static_cast<int>(lua_tonumber(s,LUA_GLOBALSINDEX-1));
    const auto function=functions[id];
    // Destruct C++ objects before lua_error's longjmp.
    bool failed=false;
    {
        std::string error;
        if(lua_gettop(s)!=function.count) error="Expected exactly "+std::to_string(function.count)+" args, but was given "+std::to_string(lua_gettop(s))+".";
        else for(int i=1;i<=function.count;++i) {
            int expected=id<7?4:3;
            if(lua_type(s,i)!=expected) {error="Expected value at stack index "+std::to_string(i)+" to be of type "+kind(expected)+" but it is "+kind(lua_type(s,i));break;}
        }
        if(!error.empty()) {error="A D function threw an exception: "+error;lua_pushstring(s,error.c_str());failed=true;}
    }
    if(failed)return luaL_error(s,"%s",lua_tolstring(s,-1,nullptr));
    double result=0;
    if(id<7) {
        auto space=insCurrentTrackingSpace();auto zone=space?space->currentZone():nullptr;
        if(zone) {
            auto name=text(s,1);
            if(id==0)result=zone->getBlendshapeFor(name);
            else {auto b=zone->getBoneFor(name);switch(id){case 1:result=b.position.x;break;case 2:result=b.position.y;break;case 3:result=b.position.z;break;case 4:result=(180/std::numbers::pi)*b.rotation.roll();break;case 5:result=(180/std::numbers::pi)*b.rotation.pitch();break;case 6:result=(180/std::numbers::pi)*b.rotation.yaw();break;}}
        }
    } else if(id==7)result=nicxlive::core::currentTime();
    else {
        std::array<float,5> v{};for(int i=0;i<function.count;++i)v[i]=static_cast<float>(lua_tonumber(s,i+1));
        auto sin=[](float x){return static_cast<float>(std::sin(double(x)));};
        auto cos=[](float x){return static_cast<float>(std::cos(double(x)));};
        auto tan=[](float x){return static_cast<float>(std::tan(double(x)));};
        switch(id) {
        case 8:result=sin(v[0]);break;case 9:result=cos(v[0]);break;case 10:result=tan(v[0]);break;
        case 11:result=dcompat::sinh(v[0]);break;case 12:result=static_cast<float>(std::cosh(double(v[0])));break;case 13:result=dcompat::tanh(v[0]);break;
        case 14:result=std::clamp(sin(v[0]),0.f,1.f);break;case 15:result=std::clamp(cos(v[0]),0.f,1.f);break;case 16:result=std::clamp(tan(v[0]),0.f,1.f);break;
        case 17:result=(1.0+sin(v[0]))/2.0;break;case 18:result=(1.0+cos(v[0]))/2.0;break;case 19:result=(1.0+tan(v[0]))/2.0;break;
        case 20:result=std::abs(v[0]);break;case 21:result=std::sqrt(v[0]);break;case 22:result=std::floor(v[0]);break;case 23:result=std::ceil(v[0]);break;case 24:result=std::round(v[0]);break;
        case 25:result=std::min(v[0],v[1]);break;case 26:result=std::max(v[0],v[1]);break;case 27:result=std::clamp(v[0],v[1],v[2]);break;
        case 28:result=v[0]*(1-v[2])+v[1]*v[2];break;
        case 29:{float t=v[4],cube=static_cast<float>(double(t)*(double(t)*double(t))),square=t*t;float h1=2*cube-3*square+1,h2=-2*cube+3*square,h3=cube-2*square+t,h4=cube-square;result=h1*v[0]+h3*v[1]+h2*v[2]+h4*v[3];break;}
        case 30:result=dcompat::atan2(v[0],v[1]);break;
        case 31:result=(180/std::numbers::pi)*v[0];break;case 32:result=(std::numbers::pi/180)*v[0];break;
        case 33:result=math::osnoise2(v[0],0);break;case 34:result=(1+math::osnoise2(v[0],0))/2.0;break;
        case 35:result=v[0]>0?1.f:v[0]==0?0.f:-1.f;break;
        }
    }
    lua_pushnumber(s,result);return 1;
}
}
void insInitExpressions() {
    std::random_device random;uint64_t seed;
    do {seed=(uint64_t(random())<<32)|random();}while(seed==UINT64_MAX);
    math::osseed(seed);
    state=luaL_newstate();
    for(size_t i=0;i<std::size(functions);++i) {lua_pushnumber(state,static_cast<double>(i));lua_pushcclosure(state,callback,1);lua_setfield(state,LUA_GLOBALSINDEX,functions[i].name);}
}
void insCleanupExpressions() {lua_close(state);state=nullptr;}
Expression::Expression(std::string signature,std::string source):exprName_(std::move(signature)),expressionSource_(std::move(source)) {
    insLogInfo("Created expression %s...",exprName_);updateState();
}
bool Expression::updateState() {
    if(expressionSource_.empty()) {lastError_="Expression is empty";return false;}
    std::string code="function "+exprName_+"() return ("+expressionSource_+") end";
    int status=luaL_loadstring(state,code.c_str());if(status==0)status=lua_pcall(state,0,-1,0);
    if(status!=0) {lastError_=text(state,-1);lua_pop(state,1);return false;}
    lastError_.clear();return true;
}
float Expression::call() {
    if(!lastError_.empty())return 0;
    int stackStartPos=lua_gettop(state);
    lua_getfield(state,LUA_GLOBALSINDEX,exprName_.c_str());
    if(lua_pcall(state,0,1,0)!=0) {lastError_=text(state,-1);return 0;}
    int returned=lua_gettop(state)-stackStartPos,type=lua_type(state,-1);
    // D's type.text resolves to the enum member, not std.conv.text(type).
    if(type!=3) {lua_pop(state,1);lastError_=i18n::_("Expected %s, got %s");for(const char* value:{"number","text"}){auto at=lastError_.find("%s");if(at!=std::string::npos)lastError_.replace(at,2,value);}return 0;}
    float value=static_cast<float>(lua_tonumber(state,-returned));lua_pop(state,returned);return value;
}
std::string insExpressionGenerateSignature(uint32_t uuid,int axis) {return "p"+std::to_string(uuid)+"_"+std::to_string(axis);}
}
