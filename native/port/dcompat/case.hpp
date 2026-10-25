#pragma once
#include "string.hpp"
#include "case_table.hpp"
#include <algorithm>
#include <span>
namespace nijiexpose::dcompat {
inline std::string changeCase(std::string_view input,std::span<const CasePair> table){
    (void)toUTF16z(input); // D's string case conversion rejects malformed UTF-8.
    std::string output;
    for(size_t i=0;i<input.size();){
        auto start=i;auto c=static_cast<uint8_t>(input[i++]);uint32_t cp=c;
        if(c>=0x80){int count=c<0xe0?1:c<0xf0?2:3;cp=c&((1u<<(6-count))-1);while(count--)cp=(cp<<6)|(static_cast<uint8_t>(input[i++])&63);}
        auto found=std::lower_bound(table.begin(),table.end(),cp,[](const CasePair& a,uint32_t b){return a.from<b;});
        if(found!=table.end()&&found->from==cp)output+=found->to;else output+=input.substr(start,i-start);
    }
    return output;
}
inline std::string toLower(std::string_view s){return changeCase(s,lowerStrings);}
inline std::string toUpper(std::string_view s){return changeCase(s,upperStrings);}
}
