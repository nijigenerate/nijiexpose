#include "nijiexpose/tracking/expr.hpp"
#include "nijiexpose/tracking/context.hpp"
#include "ft/noise.hpp"
#include <core/timing.hpp>
#include <iostream>
#include <bit>
namespace nijiexpose::tracking {
std::shared_ptr<VirtualSpace> insCurrentTrackingSpace() {static auto space=std::make_shared<VirtualSpace>();return space;}
}
int main() {
    using namespace nijiexpose::tracking;
    nicxlive::core::inSetTimingFunc([]{return 1.25;});
    nicxlive::core::inUpdate();
    insInitExpressions();nijiexpose::math::osseed(12345);
    for(std::string line;std::getline(std::cin,line);) {
        Expression expr(insExpressionGenerateSignature(123,0),line);
        float value=expr.call();std::cout<<std::bit_cast<uint32_t>(value)<<'|'<<expr.lastError()<<'\n';
    }
    insCleanupExpressions();
}
