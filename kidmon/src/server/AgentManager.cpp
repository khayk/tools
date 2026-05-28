#include <kidmon/server/AgentManager.h>
#include <core/network/TcpServer.h>

#include <spdlog/spdlog.h>

namespace km {

namespace {

core::tcp::Communicator& communicator(core::tcp::Connection& conn)
{
    auto& underlying = static_cast<AgentConnection&>(conn);
    return underlying.communicator();
}

} // namespace

AgentManager::AgentManager(AuthorizationHandler& authHandler,
                           DataHandler& dataHandler,
                           core::tcp::Server& svr,
                           std::chrono::milliseconds peerDropTimeout)
{
    svr.onCreateConnection([this, &authHandler, &dataHandler, peerDropTimeout](
                               core::tcp::Socket&& socket) {
        auto conn = std::make_shared<AgentConnection>(authHandler,
                                                      dataHandler,
                                                      std::move(socket),
                                                      peerDropTimeout);

        std::weak_ptr<AgentConnection> weakConn = conn;
        conn->onAuth([this, weakConn](AgentConnection* conn, bool auth) {
            if (authAgentConn_.expired() && auth)
            {
                spdlog::info("Agent successfully authorized: {}", fmt::ptr(conn));
                authAgentConn_ = weakConn;
                return true;
            }

            if (authAgentConn_.lock().get() == conn && !auth)
            {
                spdlog::info("Authorized agent disconnected: {}", fmt::ptr(conn));
                authAgentConn_.reset();
                return true;
            }

            return false;
        });

        return conn;
    });

    svr.onConnection([](core::tcp::Connection& conn) {
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
    return !authAgentConn_.expired();
}

} // namespace km
