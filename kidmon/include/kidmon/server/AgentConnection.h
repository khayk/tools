#pragma once

#include <core/network/TcpConnection.h>
#include <core/network/TcpCommunicator.h>
#include <cstdint>

namespace km {

class AuthorizationHandler;
class DataHandler;

class AgentConnection : public core::tcp::Connection
{
public:
    using AuthorizationCb = std::function<bool(AgentConnection* conn, bool)>;

    enum State : std::uint8_t
    {
        Disconnected,
        Connected,
        Authorized
    };

    AgentConnection(AuthorizationHandler& authHandler,
                    DataHandler& dataHandler,
                    core::tcp::Socket&& socket,
                    std::chrono::milliseconds peerDropTimeout);

    ~AgentConnection();

    void onAuth(AuthorizationCb authCb);

    State state() const noexcept;

    core::tcp::Communicator& communicator();

private:
    AuthorizationHandler& authHandler_;
    DataHandler& dataHandler_;
    State currentState_ {State::Connected};

    core::tcp::Communicator comm_;
    AuthorizationCb authCb_;

    /**
     * @brief Handle one inbound message: parse, route to the auth/data handler,
     *        and reply. Heartbeats are acknowledged without logging or
     *        persisting anything.
     */
    void onMessage(const std::string& msg);

    /**
     * @brief Advance the connection state machine.
     *
     * @return false if the transition was vetoed (e.g. the manager already has
     *         an authorized agent, so this connection must not become
     *         Authorized); true otherwise.
     */
    bool transitionTo(State newState) noexcept;
};

} // namespace km
