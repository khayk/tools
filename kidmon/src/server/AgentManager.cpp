#include <kidmon/server/AgentManager.h>
#include <core/network/TcpServer.h>

#include <spdlog/spdlog.h>

namespace {

tcp::Communicator& communicator(tcp::Connection& conn)
{
    auto& underlying = static_cast<AgentConnection&>(conn);
    return underlying.communicator();
}

} // namespace

AgentManager::AgentManager(AuthorizationHandler& authHandler,
                           DataHandler& dataHandler,
                           tcp::Server& svr,
                           std::chrono::milliseconds peerDropTimeout)
{
    svr.onCreateConnection(
        [this, &authHandler, &dataHandler, peerDropTimeout](tcp::Socket&& socket) {
            auto conn = std::make_shared<AgentConnection>(authHandler,
                                                          dataHandler,
                                                          std::move(socket),
                                                          peerDropTimeout);

            conn->onAuth([this](AgentConnection* conn, bool auth) {
                if (authAgentConn_ == nullptr && auth)
                {
                    spdlog::info("Agent successfully authorized: {}", fmt::ptr(conn));
                    authAgentConn_ = conn;
                    return true;
                }

                if (authAgentConn_ == conn && !auth)
                {
                    spdlog::info("Authorized agent disconnected: {}", fmt::ptr(conn));
                    authAgentConn_ = nullptr;
                    return true;
                }

                return false;
            });

            return conn;
        });

    svr.onConnection([](tcp::Connection& conn) {
        spdlog::info("Accepted: {}", fmt::ptr(&conn));
        communicator(conn).start();
    });

    svr.onListening([](uint16_t port) {
        spdlog::info("Listening on a port: {}", port);
    });

    svr.onClose([]() {
        spdlog::trace("Server closed");
    });

    svr.onError([](const ErrorCode& ec) {
        spdlog::error("Server error - ec: {}, msg: {}", ec.value(), ec.message());
    });
}

bool AgentManager::hasAuthorizedAgent() const
{
    return authAgentConn_ != nullptr;
}
