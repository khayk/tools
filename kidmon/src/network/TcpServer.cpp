#include "TcpServer.h"

namespace tcp {

Server::Server(IoContext& ioc)
    : ioc_(ioc)
    , acceptor_(ioc_)
    , socket_(ioc_)
{
}

Server::~Server() {
    close();
}

void Server::onListening(ListeningCb listenCb)
{
    listenCb_.add(std::move(listenCb));
}

void Server::onConnection(ConnectionCb connCb)
{
    connCb_.add(std::move(connCb));
}

void Server::onClose(CloseCb closeCb)
{
    closeCb_.add(std::move(closeCb));
}

void Server::onError(ErrorCb errorCb)
{
    errorCb_.add(std::move(errorCb));
}

void Server::listen(const Options& opts)
{
    close();

    try
    {
        Endpoint endpoint {net::ip::address_v4::loopback(), opts.port};

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(opts.reuseAddress));
        acceptor_.bind(endpoint);
        acceptor_.listen(net::socket_base::max_listen_connections);

        listenCb_(opts.port);
        listening_ = true;

        doAccept();
    }
    catch (const boost::system::system_error& ex)
    {
        errorCb_(ex.code());
    }
}

void Server::close()
{
    if (listening_)
    {
        listening_ = false;
        closeCb_();
    }

    if (acceptor_.is_open())
    {
        ErrorCode ec {};
        acceptor_.close(ec);
    }
}

void Server::doAccept()
{
    acceptor_.async_accept(socket_, [this](const ErrorCode ec) {
        if (ec)
        {
            errorCb_(ec);
            return;
        }

        auto conn = std::make_shared<Connection>(std::move(socket_));
        connCb_(*conn);

        doAccept();
    });
}

} // namespace tcp