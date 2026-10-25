#pragma once
// Phobos 2.111 std.math.trigonometry single-precision algorithms (BSL-1.0).
#include <cmath>
#include <limits>
#include <numbers>
namespace dcompat {
inline float atan(float x) {
    constexpr float pi2=static_cast<float>(std::numbers::pi/2),pi4=static_cast<float>(std::numbers::pi/4);
    if(x==0)return x;
    if(std::isinf(x))return std::copysign(pi2,x);
    bool sign=std::signbit(x);if(sign)x=-x;
    float y;
    if(x>2.4142135623730950488f){y=pi2;x=-(1.f/x);}
    else if(x>0.4142135623730950488f){y=pi4;x=(x-1.f)/(x+1.f);}
    else y=0;
    float z=x*x;
    float p=((8.05374449538E-2f*z-1.38776856032E-1f)*z+1.99777106478E-1f)*z-3.33329491539E-1f;
    y+=p*z*x+x;return sign?-y:y;
}
inline float atan2(float y,float x) {
    constexpr float pi=static_cast<float>(std::numbers::pi),pi2=static_cast<float>(std::numbers::pi/2),pi4=static_cast<float>(std::numbers::pi/4);
    if(std::isnan(x)||std::isnan(y))return std::numeric_limits<float>::quiet_NaN();
    if(y==0)return x>=0&&!std::signbit(x)?std::copysign(0.f,y):std::copysign(pi,y);
    if(x==0)return std::copysign(pi2,y);
    if(std::isinf(x)) {
        if(std::signbit(x))return std::isinf(y)?std::copysign(3*pi4,y):std::copysign(pi,y);
        return std::isinf(y)?std::copysign(pi4,y):std::copysign(0.f,y);
    }
    if(std::isinf(y))return std::copysign(pi2,y);
    float z=atan(y/x);if(std::signbit(x)){if(std::signbit(y))z-=pi;else z+=pi;}
    return z==0?std::copysign(z,y):z;
}
inline float sinh(float x) {
    if(std::abs(x)>24*static_cast<float>(std::numbers::ln2))return std::copysign(0.5f*static_cast<float>(std::exp(double(std::abs(x)))),x);
    float y=static_cast<float>(std::expm1(double(x)));return 0.5f*y/(y+1)*(y+2);
}
inline float tanh(float x) {
    if(std::abs(x)>24*static_cast<float>(std::numbers::ln2))return std::copysign(1.f,x);
    float y=static_cast<float>(std::expm1(double(2*x)));return y/(y+2);
}
}
