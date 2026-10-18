#pragma once
#include <core/math/types.hpp>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
namespace nijiexpose::ft {
using Vec3=nicxlive::core::math::Vec3;
// inmath quaternion component order is w,x,y,z.
struct Quaternion {
    float w=1,x=0,y=0,z=0;
    double yaw() const { return std::atan2(2.0*(w*z+x*y),1.0-2.0*(y*y+z*z)); }
    double pitch() const { return std::asin(2.0*(w*y-z*x)); }
    double roll() const { return std::atan2(2.0*(w*x+y*z),1.0-2.0*(x*x+y*y)); }
};
struct Bone { Vec3 position{0,0,0}; Quaternion rotation{}; };
class Adaptor {
protected:
    std::map<std::string,float> blendshapes;
    std::map<std::string,Bone> bones;
    std::map<std::string,std::string> options;
    std::atomic<int> dataLossCounter=0;
    static constexpr int RECV_TIMEOUT=16;
public:
    virtual ~Adaptor()=default;
    virtual void start()=0;
    virtual void stop()=0;
    virtual void poll()=0;
    virtual std::vector<std::string> getOptionNames()=0;
    virtual bool isRunning()=0;
    virtual bool isReceivingData()=0;
    virtual std::string getAdaptorName()=0;
    void start(std::map<std::string,std::string> value) { setOptions(std::move(value)); start(); }
    void setOptions(std::map<std::string,std::string> value={}) { options=std::move(value); }
    auto& getOptions() { return options; }
    auto& getBlendshapes() { return blendshapes; }
    auto& getBones() { return bones; }
};
}
