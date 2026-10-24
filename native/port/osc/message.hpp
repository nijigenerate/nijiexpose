#pragma once
#include <bit>
#include <cstdint>
#include <span>
#include <string>
#include <stdexcept>
#include <vector>
#include <type_traits>
namespace nijiexpose::osc {
struct Message {
    std::vector<std::string> addressPattern;
    std::string typeTags;
    std::vector<std::vector<uint8_t>> args;
    explicit Message(std::span<const uint8_t> bytes);
    Message()=default;
    template<class T> T arg(size_t index) const {
        const auto& bytes=args.at(index);
        if constexpr(std::is_same_v<T,std::string>) {
            size_t length=bytes.size(); while(length && bytes[length-1]==0) --length;
            return {reinterpret_cast<const char*>(bytes.data()),length};
        } else {
            if(bytes.size()<4) throw std::out_of_range("OSC argument range");
            uint32_t bits=(uint32_t(bytes[0])<<24)|(uint32_t(bytes[1])<<16)|(uint32_t(bytes[2])<<8)|bytes[3];
            return std::bit_cast<T>(bits);
        }
    }
};
std::vector<Message> messages(std::span<const uint8_t> packet);
}
