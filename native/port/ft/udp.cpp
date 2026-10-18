#define NOMINMAX
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
using SOCKET=int;
#endif
#include "udp.hpp"
#include <stdexcept>
namespace nijiexpose::ft {
namespace {
#ifdef _WIN32
struct Winsock { Winsock() { WSADATA data; if(WSAStartup(MAKEWORD(2,2),&data)) throw std::runtime_error("WSAStartup failed"); } ~Winsock() { WSACleanup(); } };
#else
struct Winsock {};
#endif
sockaddr_in resolve(const std::string& host,uint16_t port) {
    sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(port);
    if(inet_pton(AF_INET,host.c_str(),&addr.sin_addr)!=1) {
        addrinfo hints{}; hints.ai_family=AF_INET; hints.ai_socktype=SOCK_DGRAM;
        addrinfo* result=nullptr;
        if(getaddrinfo(host.c_str(),nullptr,&hints,&result)!=0 || !result) throw std::runtime_error("Cannot resolve tracking address: "+host);
        addr.sin_addr=reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr; freeaddrinfo(result);
    }
    return addr;
}
}
UdpSocket::UdpSocket() {
    static Winsock winsock;
    socket=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(socket==INVALID_SOCKET) throw std::runtime_error("Cannot create tracking socket");
}
UdpSocket::~UdpSocket() { close(); }
void UdpSocket::bind(const std::string& host,uint16_t port) {
    auto address=resolve(host,port);
    if(::bind(socket,reinterpret_cast<sockaddr*>(&address),sizeof(address))==SOCKET_ERROR) throw std::runtime_error("Cannot bind tracking socket");
}
void UdpSocket::timeout(unsigned milliseconds) {
#ifdef _WIN32
    DWORD timeout=milliseconds;
    if(setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout))==SOCKET_ERROR) throw std::runtime_error("Cannot set tracking socket timeout");
#else
    timeval timeout{static_cast<time_t>(milliseconds/1000),static_cast<suseconds_t>((milliseconds%1000)*1000)};
    if(setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==SOCKET_ERROR) throw std::runtime_error("Cannot set tracking socket timeout");
#endif
}
int UdpSocket::receive(std::span<uint8_t> bytes) { return static_cast<int>(recv(socket,reinterpret_cast<char*>(bytes.data()),bytes.size(),0)); }
void UdpSocket::sendTimeout(unsigned milliseconds) {
#ifdef _WIN32
    DWORD value=milliseconds;
    if(setsockopt(socket,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&value),sizeof(value))==SOCKET_ERROR) throw std::runtime_error("Cannot set tracking send timeout");
#else
    timeval value{static_cast<time_t>(milliseconds/1000),static_cast<suseconds_t>((milliseconds%1000)*1000)};
    if(setsockopt(socket,SOL_SOCKET,SO_SNDTIMEO,&value,sizeof(value))==SOCKET_ERROR) throw std::runtime_error("Cannot set tracking send timeout");
#endif
}
int UdpSocket::sendTo(std::span<const uint8_t> bytes,const std::string& host,uint16_t port) {
    auto address=resolve(host,port);
    return static_cast<int>(sendto(socket,reinterpret_cast<const char*>(bytes.data()),bytes.size(),0,reinterpret_cast<sockaddr*>(&address),sizeof(address)));
}
void UdpSocket::close() { if(socket!=INVALID_SOCKET) {
#ifdef _WIN32
    closesocket(socket);
#else
    ::close(socket);
#endif
    socket=INVALID_SOCKET;
} }
}
