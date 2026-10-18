#pragma once
#include <cstdint>
#include <span>
#include <string>
namespace nijiexpose::ft {
// Windows implementation of the std.socket calls used by the D adaptors.
class UdpSocket {
    uintptr_t socket;
public:
    UdpSocket();
    ~UdpSocket();
    UdpSocket(const UdpSocket&)=delete;
    UdpSocket& operator=(const UdpSocket&)=delete;
    void bind(const std::string& address,uint16_t port);
    void timeout(unsigned milliseconds);
    void sendTimeout(unsigned milliseconds);
    int receive(std::span<uint8_t> bytes);
    int sendTo(std::span<const uint8_t> bytes,const std::string& address,uint16_t port);
    void close();
};
}
