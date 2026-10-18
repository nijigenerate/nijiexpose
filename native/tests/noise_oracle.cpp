#include "ft/noise.hpp"
#include <bit>
#include <iostream>
int main() {
    for(uint64_t seed:{0ULL,1ULL,123456789ULL,UINT64_MAX}) {
        nijiexpose::math::osseed(seed);
        for(int i=-100;i<101;++i) std::cout<<std::bit_cast<uint64_t>(nijiexpose::math::osnoise2(i*0.125,i*0.03))<<'\n';
    }
}
