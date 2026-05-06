#include <core/network/TcpClient.h>

namespace tcp {

Client::Client(IoContext& ioc) noexcept
    : socket_ {ioc}
{
}

void Client::onConnect(ConnectCb connectCb)
{
    connectCb_.add(std::move(connectCb));
}

void Client::onError(ErrorCb errorCb)
{
    errorCb_.add(std::move(errorCb));
}

void Client::connect(const Options& opts)
{
    // @todo:hayk - resolve host -> ip later, now assume it is a valid ip
    // https://theboostcpplibraries.com/boost.asio-network-programming
    const Endpoint endpoint(net::ip::make_address(opts.host), opts.port);

    socket_.async_connect(endpoint, [this](const ErrorCode& ec) {
        if (ec)
        {
            errorCb_(ec);
            return;
        }

        auto conn = std::make_shared<Connection>(std::move(socket_));
        connectCb_(*conn);
    });
}

} // namespace tcp