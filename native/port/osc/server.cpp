// Windows socket adaptation of vmc-d 1.1.3 osc.server.
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include "server.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include <limits>
namespace nijiexpose::osc {
namespace {
struct Winsock {
    Winsock() { WSADATA data; if(WSAStartup(MAKEWORD(2,2),&data)) throw std::runtime_error("WSAStartup failed"); }
    ~Winsock() { WSACleanup(); }
};
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
    DWORD timeout=16;
    if(setsockopt(impl->socket,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout))==SOCKET_ERROR) {
        closesocket(impl->socket); throw std::runtime_error("Cannot set tracking socket timeout");
    }
    sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(port);
    if(inet_pton(AF_INET,address.c_str(),&addr.sin_addr)!=1) {
        addrinfo hints{}; hints.ai_family=AF_INET; hints.ai_socktype=SOCK_DGRAM;
        addrinfo* resolved=nullptr;
        if(getaddrinfo(address.c_str(),nullptr,&hints,&resolved)!=0 || !resolved) {
            closesocket(impl->socket); throw std::runtime_error("Cannot resolve tracking address: "+address);
        }
        addr.sin_addr=reinterpret_cast<sockaddr_in*>(resolved->ai_addr)->sin_addr;
        freeaddrinfo(resolved);
    }
    if(::bind(impl->socket,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))==SOCKET_ERROR) {
        closesocket(impl->socket); throw std::runtime_error("Cannot bind tracking socket: "+address+":"+std::to_string(port));
    }
    impl->shouldRun=true;
    impl->thread=std::thread([this] {
        while(impl->shouldRun) {
            int size=recv(impl->socket,reinterpret_cast<char*>(impl->recvBuffer.data()),static_cast<int>(impl->recvBuffer.size()),0);
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
    if(impl->socket!=INVALID_SOCKET) { closesocket(impl->socket); impl->socket=INVALID_SOCKET; }
}
}
