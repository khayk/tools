#pragma once

#include "AgentConnection.h"

#include <chrono>
#include <optional>

namespace core::tcp {
class Server;
}

namespace km {

class AgentManager
{
    std::weak_ptr<AgentConnection> authAgentConn_;
    std::chrono::steady_clock::time_point authorizedAt_;

public:
    AgentManager(AuthorizationHandler& authHandler,
                 DataHandler& dataHandler,
                 core::tcp::Server& svr,
                 std::chrono::milliseconds peerDropTimeout);

    [[nodiscard]] bool hasAuthorizedAgent() const;

    // How long the current agent has been authorized; nullopt if there is none.
    [[nodiscard]] std::optional<std::chrono::milliseconds> authorizedFor() const;
};

} // namespace km
