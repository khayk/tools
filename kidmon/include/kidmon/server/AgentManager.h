#pragma once

#include "AgentConnection.h"

namespace core::tcp {
class Server;
}

namespace km {

class AgentManager
{
    AgentConnection* authAgentConn_ {nullptr};

public:
    AgentManager(AuthorizationHandler& authHandler,
                 DataHandler& dataHandler,
                 core::tcp::Server& svr,
                 std::chrono::milliseconds peerDropTimeout);

    [[nodiscard]] bool hasAuthorizedAgent() const;
};

} // namespace km
