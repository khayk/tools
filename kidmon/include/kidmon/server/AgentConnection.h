#pragma once

#include <core/network/TcpConnection.h>
#include <core/network/TcpCommunicator.h>

class AuthorizationHandler;
class DataHandler;

class AgentConnection : public tcp::Connection
{
public:
    using AuthorizationCb = std::function<bool(AgentConnection* conn, bool)>;

    enum Status
    {
        Disconnected,
        Connected,
        Authorized
    };

    AgentConnection(AuthorizationHandler& authHandler,
                    DataHandler& dataHandler,
                    tcp::Socket&& socket);

    ~AgentConnection();

    void onAuth(AuthorizationCb authCb);

    Status status() const noexcept;

    tcp::Communicator& communicator();

private:
    AuthorizationHandler& authHandler_;
    DataHandler& dataHandler_;
    Status status_ {Status::Connected};

    tcp::Communicator comm_;
    AuthorizationCb authCb_;

    void transitionTo(Status status) noexcept;
};
