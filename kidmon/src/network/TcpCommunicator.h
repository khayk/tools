#pragma once

#include "TcpConnection.h"
#include <network/data/Unpacker.h>
#include <network/data/Packer.h>
#include <network/data/StringSource.h>

#include <queue>

namespace tcp {

class Communicator
{
public:
    using MsgCb        = std::function<void(const std::string&)>;

    Communicator(Connection& conn);
    ~Communicator();

    bool onMsg(MsgCb msgCb);
    void start();
    void send(std::string_view msg);

private:
    MsgCb msgCb_;
    Connection* conn_ {nullptr};
    data::Unpacker unpacker_;
    std::queue<std::string> wq_;
    std::string msg_;
    size_t off_ {0};
    bool started_ {false};
    bool sending_ {false};

    const std::string& sendBuf() const;
    void onRead(const char* data, size_t size);
    void onSent(size_t size);
    void sendInternal();
};

} // namespace tcp
