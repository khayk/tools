#pragma once

#include "AgentConnection.h"

namespace tcp {
class Server;
}

class AgentManager
{
    AgentConnection* authAgentConn_ {nullptr};

public:
    AgentManager(AuthorizationHandler& authHandler,
                 DataHandler& dataHandler,
                 tcp::Server& svr);

    [[nodiscard]] bool hasAuthorizedAgent() const;
};
