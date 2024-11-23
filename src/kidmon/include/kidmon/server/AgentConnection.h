#pragma once

#include <core/network/TcpConnection.h>
#include <core/network/TcpCommunicator.h>

class AuthorizationHandler;
class DataHandler;

class AgentConnection : public tcp::Connection
{
public:
    using AuthorizationCb = std::function<bool(AgentConnection* conn, bool)>;

    enum State
    {
        Disconnected,
        Connected,
        Authorized
    };

    AgentConnection(AuthorizationHandler& authHandler,
                    DataHandler& dataHandler,
                    tcp::Socket&& socket,
                    std::chrono::milliseconds peerDropTimeout);

    ~AgentConnection();

    void onAuth(AuthorizationCb authCb);

    State state() const noexcept;

    tcp::Communicator& communicator();

private:
    AuthorizationHandler& authHandler_;
    DataHandler& dataHandler_;
    State currentState_ {State::Connected};

    tcp::Communicator comm_;
    AuthorizationCb authCb_;

    void transitionTo(State newState) noexcept;
};
