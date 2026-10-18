// Windows socket adaptation of vmc-d 1.1.3 osc.server.
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
#include "server.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include <limits>
namespace nijiexpose::osc {
namespace {
#ifdef _WIN32
struct Winsock {
    Winsock() { WSADATA data; if(WSAStartup(MAKEWORD(2,2),&data)) throw std::runtime_error("WSAStartup failed"); }
    ~Winsock() { WSACleanup(); }
};
#else
struct Winsock {};
#endif
}
struct Server::Impl {
    SOCKET socket=INVALID_SOCKET;
    std::atomic<bool> shouldRun=false;
    std::thread thread;
    std::mutex mutex;
    std::vector<Message> contents;
    std::vector<uint8_t> recvBuffer=std::vector<uint8_t>(std::numeric_limits<uint16_t>::max());
};
Server::Server(uint16_t port):Server("0.0.0.0",port) {}
Server::Server(const std::string& address,uint16_t port):impl(std::make_unique<Impl>()) {
    static Winsock winsock;
    impl->socket=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(impl->socket==INVALID_SOCKET) throw std::runtime_error("Cannot create tracking socket");
#ifdef _WIN32
    DWORD timeout=16;
    const auto timeoutResult=setsockopt(impl->socket,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));
#else
    timeval timeout{0,16000};
    const auto timeoutResult=setsockopt(impl->socket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
#endif
    if(timeoutResult==SOCKET_ERROR) {
#ifdef _WIN32
        closesocket(impl->socket); throw std::runtime_error("Cannot set tracking socket timeout");
#else
        ::close(impl->socket); throw std::runtime_error("Cannot set tracking socket timeout");
#endif
    }
    sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(port);
    if(inet_pton(AF_INET,address.c_str(),&addr.sin_addr)!=1) {
        addrinfo hints{}; hints.ai_family=AF_INET; hints.ai_socktype=SOCK_DGRAM;
        addrinfo* resolved=nullptr;
        if(getaddrinfo(address.c_str(),nullptr,&hints,&resolved)!=0 || !resolved) {
#ifdef _WIN32
            closesocket(impl->socket);
#else
            ::close(impl->socket);
#endif
            throw std::runtime_error("Cannot resolve tracking address: "+address);
        }
        addr.sin_addr=reinterpret_cast<sockaddr_in*>(resolved->ai_addr)->sin_addr;
        freeaddrinfo(resolved);
    }
    if(::bind(impl->socket,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))==SOCKET_ERROR) {
#ifdef _WIN32
        closesocket(impl->socket);
#else
        ::close(impl->socket);
#endif
        throw std::runtime_error("Cannot bind tracking socket: "+address+":"+std::to_string(port));
    }
    impl->shouldRun=true;
    impl->thread=std::thread([this] {
        while(impl->shouldRun) {
            int size=static_cast<int>(recv(impl->socket,reinterpret_cast<char*>(impl->recvBuffer.data()),impl->recvBuffer.size(),0));
            if(size!=SOCKET_ERROR) {
                auto packet=messages(std::span<const uint8_t>(impl->recvBuffer.data(),size));
                std::lock_guard lock(impl->mutex);
                impl->contents.insert(impl->contents.end(),packet.begin(),packet.end());
            }
        }
    });
}
Server::~Server() { close(); }
std::vector<Message> Server::popMessages() {
    std::lock_guard lock(impl->mutex);
    auto result=std::move(impl->contents); impl->contents.clear(); return result;
}
void Server::close() {
    if(impl->thread.joinable()) { impl->shouldRun=false; impl->thread.join(); }
    if(impl->socket!=INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(impl->socket);
#else
        ::close(impl->socket);
#endif
        impl->socket=INVALID_SOCKET;
    }
}
}
