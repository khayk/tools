#include <core/network/TcpConnection.h>

namespace tcp {

Connection::Connection(Socket socket) noexcept
    : socket_ {std::move(socket)}
    , data_ {}
{
}

Connection::~Connection()
{
    disconnectCb_();
}

void Connection::onRead(ReadCb readCb)
{
    readCb_.add(std::move(readCb));
}

void Connection::onSent(SentCb sentCb)
{
    sentCb_.add(std::move(sentCb));
}

void Connection::onError(ErrorCb errorCb)
{
    errorCb_.add(std::move(errorCb));
}

void Connection::onDisconnect(DisconnectCb disconnectCb)
{
    disconnectCb_.add(std::move(disconnectCb));
}

void Connection::read()
{
    auto self {shared_from_this()};

    socket_.async_read_some(net::buffer(data_),
                            [this, self](const ErrorCode& ec, std::size_t size) {
                                handleRead(ec, size);
                            });
}

void Connection::write(const char* data, size_t size)
{
    auto self {shared_from_this()};

    net::async_write(socket_,
                     ConstBuffer {data, size},
                     [this, self](const ErrorCode& ec, std::size_t size) {
                         handleWrite(ec, size);
                     });
}

void Connection::close()
{
    std::unique_lock guard(mutex_);

    if (socket_.is_open())
    {
        ErrorCode ec;
        socket_.shutdown(Socket::shutdown_both, ec);
        socket_.close(ec);
    }
}

void Connection::write(std::string_view sv)
{
    write(sv.data(), sv.size());
}

void Connection::handleRead(const ErrorCode& ec, std::size_t size)
{
    if (ec)
    {
        errorCb_(ec);
        return;
    }

    readCb_(data_.data(), size);
}

void Connection::handleWrite(const ErrorCode& ec, size_t size)
{
    if (ec)
    {
        errorCb_(ec);
        return;
    }

    sentCb_(size);
}

} // namespace tcp