#pragma once

#include <core/patterns/Callback.h>

#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>

#include <memory>
#include <array>

namespace net       = boost::asio;
using ErrorCode     = boost::system::error_code;
using IoContext     = net::io_context;
using ConstBuffer   = net::const_buffer;

namespace tcp {

using Socket        = net::ip::tcp::socket;
using Endpoint      = net::ip::tcp::endpoint;

class Connection
    : public std::enable_shared_from_this<Connection>
{
    using ReadCbs       = dp::Callback<const char*, size_t>;
    using SentCbs       = dp::Callback<size_t>;
    using ErrorCbs      = dp::Callback<const ErrorCode&>;
    using DisconnectCbs = dp::Callback<>;

public:
    using ReadCb        = ReadCbs::Function;
    using SentCb        = SentCbs::Function;
    using ErrorCb       = ErrorCbs::Function;
    using DisconnectCb  = DisconnectCbs::Function;

    explicit Connection(Socket socket, uint16_t bufferSize = 4096) noexcept;
    ~Connection();

    void onRead(ReadCb readCb);
    void onSent(SentCb sentCb);
    void onError(ErrorCb errorCb);
    void onDisconnect(DisconnectCb disconnectCb);

    void read();
    void write(std::string_view sv);
    void write(const char* data, size_t size);
    void close();

private:
    void handleRead(const ErrorCode& ec, std::size_t size);
    void handleWrite(const ErrorCode& ec, size_t size);

    Socket socket_;
    ReadCbs readCb_;
    SentCbs sentCb_;
    ErrorCbs errorCb_;
    DisconnectCbs disconnectCb_;

    std::mutex mutex_;
    std::string data_;
};

} // namespace tcp