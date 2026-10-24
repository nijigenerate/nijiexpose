#include "ft/noise.hpp"
#include <bit>
#include <iostream>
#include <initializer_list>
int main() {
    for(uint64_t seed:std::initializer_list<uint64_t>{0,1,123456789,UINT64_MAX}) {
        nijiexpose::math::osseed(seed);
        for(int i=-100;i<101;++i) std::cout<<std::bit_cast<uint64_t>(nijiexpose::math::osnoise2(i*0.125,i*0.03))<<'\n';
    }
}
