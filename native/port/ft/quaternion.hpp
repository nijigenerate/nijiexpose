#pragma once
#include "adaptor.hpp"
#include <algorithm>
#include <numbers>
namespace nijiexpose::ft {
inline Quaternion eulerRotation(double roll,double pitch,double yaw) {
    auto cr=std::cos(roll/2.0),cp=std::cos(pitch/2.0),cy=std::cos(yaw/2.0);
    auto sr=std::sin(roll/2.0),sp=std::sin(pitch/2.0),sy=std::sin(yaw/2.0);
    return {static_cast<float>(cr*cp*cy+sr*sp*sy),static_cast<float>(sr*cp*cy-cr*sp*sy),
        static_cast<float>(cr*sp*cy+sr*cp*sy),static_cast<float>(cr*cp*sy-sr*sp*cy)};
}
inline Quaternion operator*(Quaternion a,float b) { return {a.w*b,a.x*b,a.y*b,a.z*b}; }
inline Quaternion operator+(Quaternion a,Quaternion b) { return {a.w+b.w,a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Quaternion operator*(Quaternion a,Quaternion b) {
    return {-a.x*b.x-a.y*b.y-a.z*b.z+a.w*b.w,a.x*b.w+a.y*b.z-a.z*b.y+a.w*b.x,
        -a.x*b.z+a.y*b.w+a.z*b.x+a.w*b.y,a.x*b.y-a.y*b.x+a.z*b.w+a.w*b.z};
}
inline Quaternion axisRotation(double alpha,Vec3 axis) {
    if(alpha==0) return {};
    alpha/=2; float sinaqt=static_cast<float>(std::sin(alpha));
    return {static_cast<float>(std::cos(alpha)),axis.x*sinaqt,axis.y*sinaqt,axis.z*sinaqt};
}
inline Quaternion lookRotation(Vec3 forward,Vec3 up) {
    auto normalized=[](Vec3 a) {
        double sum=0; sum+=a.x*a.x; sum+=a.y*a.y; sum+=a.z*a.z;
        double len=std::sqrt(sum);
        if(len!=0) a={static_cast<float>(a.x/len),static_cast<float>(a.y/len),static_cast<float>(a.z/len)};
        return a;
    };
    auto cross=[](Vec3 a,Vec3 b)->Vec3 { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; };
    forward=normalized(forward);
    Vec3 right=normalized(cross(up,forward)); up=cross(forward,right);
    float m00=right.x,m01=right.y,m02=right.z,m10=up.x,m11=up.y,m12=up.z,m20=forward.x,m21=forward.y,m22=forward.z;
    float num8=(m00+m11)+m22; Quaternion q;
    if(num8>0) {
        float num=static_cast<float>(std::sqrt(num8+1.0)); q.w=num*0.5f; num=0.5f/num;
        q.x=(m12-m21)*num; q.y=(m20-m02)*num; q.z=(m01-m10)*num; return q;
    }
    if(m00>=m11 && m00>=m22) {
        float num7=std::sqrt(((1.f+m00)-m11)-m22),num4=0.5f/num7;
        q.x=0.5f*num7; q.y=(m01+m10)*num4; q.z=(m02+m20)*num4; q.w=(m12-m21)*num4; return q;
    }
    if(m11>m22) {
        float num6=std::sqrt(((1.f+m11)-m00)-m22),num3=0.5f/num6;
        q.x=(m10+m01)*num3; q.y=0.5f*num6; q.z=(m21+m12)*num3; q.w=(m20-m02)*num3; return q;
    }
    float num5=std::sqrt(((1.f+m22)-m00)-m11),num2=0.5f/num5;
    q.x=(m20+m02)*num2; q.y=(m21+m12)*num2; q.z=0.5f*num5; q.w=(m01-m10)*num2; return q;
}
inline Quaternion slerp(Quaternion a,Quaternion b,float t) {
    // inmath.interpolate.slerp; real is double in the Windows D reference.
    // std.math.acos(float) evaluates in real, then rounds to float before
    // assignment to theta. Preserve this intermediate rounding.
    double theta=static_cast<float>(std::acos(static_cast<double>(std::clamp(a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z,-1.f,1.f))));
    auto equal=[](double a,double b) { return std::abs(a-b)<=0.000001f || std::abs(a-b)<=0.000001f*std::abs(b); };
    if(equal(theta,0)) return a;
    if(equal(theta,std::numbers::pi)) return a*(1-t)+b*t;
    double sintheta=std::sin(theta);
    // Quaternion scalar operator takes qt (float), even for a real operand.
    return a*static_cast<float>(std::sin((1.0-t)*theta)/sintheta)+b*static_cast<float>(std::sin(t*theta)/sintheta);
}
}
