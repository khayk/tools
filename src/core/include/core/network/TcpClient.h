#pragma once

#include <core/network/TcpConnection.h>

namespace tcp {

class Client
{
    using ConnectCbs = dp::Callback<Connection&>;
    using ErrorCbs = dp::Callback<const ErrorCode&>;

public:
    using ConnectCb = ConnectCbs::Function;
    using ErrorCb = ErrorCbs::Function;

    struct Options
    {
        std::string host;
        uint16_t port {0};
    };

    Client(IoContext& ioc) noexcept;

    void onConnect(ConnectCb connectCb);
    void onError(ErrorCb errorCb);

    void connect(const Options& opts);

private:
    Socket socket_;
    ConnectCbs connectCb_;
    ErrorCbs errorCb_;
};

} // namespace tcp