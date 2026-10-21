// vmc-d 1.1.3 osc.message/packet/bundle receive implementation.
#include "message.hpp"
#include <algorithm>
namespace nijiexpose::osc {
namespace {
size_t zero(std::span<const uint8_t> bytes) {
    auto it=std::find(bytes.begin(),bytes.end(),0);
    if(it==bytes.end()) throw std::out_of_range("Unterminated OSC string");
    return static_cast<size_t>(it-bytes.begin());
}
std::span<const uint8_t> take(std::span<const uint8_t>& bytes,size_t count) {
    if(count>bytes.size()) throw std::out_of_range("OSC data range");
    auto result=bytes.first(count); bytes=bytes.subspan(count); return result;
}
}
Message::Message(std::span<const uint8_t> bytes) {
    size_t addressLength=zero(bytes);
    if(addressLength<1) throw std::out_of_range("OSC address range");
    std::string address(reinterpret_cast<const char*>(bytes.data()+1),addressLength-1);
    size_t begin=0;
    for(;;) {
        auto end=address.find('/',begin);
        addressPattern.push_back("/"+address.substr(begin,end==std::string::npos?end:end-begin));
        if(end==std::string::npos) break; begin=end+1;
    }
    bytes=bytes.subspan(addressLength);
    while(!bytes.empty() && bytes[0]==0) bytes=bytes.subspan(1);
    if(bytes.size()%4) throw std::out_of_range("OSC remaining length is not aligned");
    bool empty=true;
    for(size_t i=0;i<bytes.size();++i) { if(i==0&&bytes[i]==44) continue; if(bytes[i]!=0) empty=false; }
    if(empty) return;
    auto tagEnd=zero(bytes);
    if(tagEnd<1) throw std::out_of_range("OSC type tag range");
    typeTags.assign(reinterpret_cast<const char*>(bytes.data()+1),tagEnd-1);
    take(bytes,(tagEnd/4)*4+4);
    if(bytes.size()%4) throw std::out_of_range("OSC argument length is not aligned");
    for(char tag:typeTags) {
        std::span<const uint8_t> data;
        switch(tag) {
        case 'i':case 'f':case 'b': data=take(bytes,4); break;
        case 's': data=take(bytes,(zero(bytes)/4)*4+4); break;
        default: throw std::logic_error("Unsupported OSC type tag in original vmc-d");
        }
        args.emplace_back(data.begin(),data.end());
    }
}
namespace {
std::vector<Message> parsePacket(std::span<const uint8_t> packet,size_t depth) {
    if(depth>32) throw std::out_of_range("OSC bundle nesting limit");
    if(packet.empty()) throw std::out_of_range("Empty OSC packet");
    if(packet[0]!=0x23) return {Message(packet)};
    take(packet,16);
    std::vector<Message> result;
    do {
        auto sizeData=take(packet,4);
        uint32_t size=0; for(auto byte:sizeData) size=(size<<8)|byte;
        auto nested=parsePacket(take(packet,size),depth+1);
        result.insert(result.end(),nested.begin(),nested.end());
    } while(!packet.empty());
    return result;
}
}
std::vector<Message> messages(std::span<const uint8_t> packet) {
    return parsePacket(packet,0);
}
}
