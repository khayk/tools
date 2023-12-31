#pragma once

#include "TcpConnection.h"

namespace tcp {

class Server
{
    using ListeningCbs   = dp::Callback<uint16_t>;
    using ConnectionCbs  = dp::Callback<Connection&>;
    using CloseCbs       = dp::Callback<>;
    using ErrorCbs       = dp::Callback<const ErrorCode&>;

public:
    using ListeningCb    = ListeningCbs::Function;
    using ConnectionCb   = ConnectionCbs::Function;
    using CloseCb        = CloseCbs::Function;
    using ErrorCb        = ErrorCbs::Function;

    struct Options
    {
        uint16_t port {0};
        bool reuseAddress {false};
    };

    Server(IoContext& ioc);
    ~Server();

    /**
     * @brief Events
     */
    void onListening(ListeningCb listenCb);
    void onConnection(ConnectionCb connCb);
    void onClose(CloseCb closeCb);
    void onError(ErrorCb errorCb);

    /**
     * @brief Operations
     */
    void listen(const Options& opts);
    void close();

private:
    void doAccept();

    using Acceptor = net::ip::tcp::acceptor;

    IoContext& ioc_;
    Acceptor acceptor_;
    Socket socket_;
    bool listening_ {false};

    ListeningCbs listenCb_;
    ConnectionCbs connCb_;
    CloseCbs closeCb_;
    ErrorCbs errorCb_;
};

} // namespace tcp
