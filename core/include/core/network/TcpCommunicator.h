#pragma once

#include <core/network/TcpConnection.h>

namespace tcp {

class Communicator
{
    static void defHandler(bool) {}

public:
    using MsgCb  = std::function<void(const std::string&)>;
    using SentCb = std::function<void(bool)>;

    Communicator(Connection& conn);
    ~Communicator();

    bool onMsg(MsgCb msgCb);

    void start();
    void sendAsync(std::string_view data, SentCb sentCb = defHandler);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace tcp
