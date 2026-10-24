#pragma once
#include "message.hpp"
#include <memory>
namespace nijiexpose::osc {
class Server {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    explicit Server(uint16_t port);
    Server(const std::string& address,uint16_t port);
    ~Server();
    std::vector<Message> popMessages();
    void close();
};
}
