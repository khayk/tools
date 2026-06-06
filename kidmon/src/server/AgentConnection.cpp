#include <kidmon/server/AgentConnection.h>
#include <kidmon/server/handler/AuthorizationHandler.h>
#include <kidmon/server/handler/DataHandler.h>
#include <kidmon/data/Messages.h>
#include <core/utils/Str.h>
#include <core/utils/Sys.h>

#include <boost/asio/error.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace km {
namespace {

auto defAuthHandler = [](AgentConnection* conn, bool) {
    spdlog::info("Default auth handler is called: {}", fmt::ptr(conn));
    return false;
};

} // namespace

AgentConnection::AgentConnection(AuthorizationHandler& authHandler,
                                 DataHandler& dataHandler,
                                 core::tcp::Socket&& sock,
                                 std::chrono::milliseconds peerDropTimeout)
    : Connection(std::move(sock))
    , authHandler_(authHandler)
    , dataHandler_(dataHandler)
    , comm_(*this)
    , authCb_(defAuthHandler)
{
    comm_.onMsg([this](const std::string& msg) {
        try
        {
            spdlog::debug("Server rcvd: {} bytes", msg.size());

            const auto payload = nlohmann::json::parse(msg);
            nlohmann::json answer;
            std::string error;

            bool handled = false;
            if (currentState_ == State::Authorized)
            {
                handled = dataHandler_.handle(payload, answer, error);
            }
            else
            {
                handled = authHandler_.handle(payload, answer, error);
                if (handled && error.empty() && !transitionTo(State::Authorized))
                {
                    // Token was valid but another agent is already authorized;
                    // refuse this connection instead of letting it inject data.
                    answer["authorized"] = false;
                    error = "An authorized agent already exists";
                }
            }

            if (!handled)
            {
                error = "Unexpected message";
            }

            // Honor the protocol contract: status 0 means the request was
            // handled, non-zero signals failure and carries the reason in
            // "error". Previously this always sent status 0, so the agent's
            // failure branch (which keys off status != 0) never fired.
            nlohmann::ordered_json js;
            const int status = error.empty() ? 0 : 1;
            msgs::buildResponse(status, error, answer, js);
            const auto res = js.dump();
            spdlog::debug("Server sent: {} bytes", res.size());
            comm_.sendAsync(res);

            if (!error.empty())
            {
                spdlog::error("Request handling failed, details: {}", error);
                transitionTo(State::Disconnected);
                close();
            }
        }
        catch (const std::exception& ex)
        {
            spdlog::error("Exception in request handling, details: {}", ex.what());
            transitionTo(State::Disconnected);
            close();
        }
    });

    onError([this](const ErrorCode& ec) {
        spdlog::error("Connection error: {}, ec: {}, msg: {}",
                      fmt::ptr(this),
                      ec.value(),
                      ec.message());
        transitionTo(State::Disconnected);
        close();
    });

    setTimeout(peerDropTimeout);
    spdlog::trace("Set timer to drop peer when it is inactive for {} ms",
                  peerDropTimeout.count());

    onTimeout([this](const ErrorCode& ec) {
        std::ignore = ec;
        spdlog::trace("No data from agent: {}", fmt::ptr(this));

        if (currentState_ == State::Connected)
        {
            spdlog::info("Dropping unauthenticated connection: {}", fmt::ptr(this));
            transitionTo(State::Disconnected);
            close();
            return;
        }

        const auto activeUsername = core::str::ws2s(core::sys::activeUserName());

        if (!authHandler_.username().empty() && !activeUsername.empty() &&
            authHandler_.username() != activeUsername)
        {
            spdlog::info("Active user changed from '{}' to '{}'. Dropping peer",
                         authHandler_.username(),
                         activeUsername);
            transitionTo(State::Disconnected);
            close();
        }
    });
}

AgentConnection::~AgentConnection()
{
    spdlog::info("Dropped: {}", fmt::ptr(this));
    transitionTo(State::Disconnected);
}

void AgentConnection::onAuth(AuthorizationCb authCb)
{
    authCb_ = std::move(authCb);
}

AgentConnection::State AgentConnection::state() const noexcept
{
    return currentState_;
}

core::tcp::Communicator& AgentConnection::communicator()
{
    return comm_;
}

bool AgentConnection::transitionTo(const State newState) noexcept
{
    try
    {
        if (currentState_ == State::Connected && newState == State::Authorized)
        {
            // The manager vetoes a second concurrent agent. Honor the veto: if
            // it refuses, stay unauthorized so this connection can never reach
            // the data path (prevents token-replay data injection while a
            // legitimate agent is already connected).
            if (!authCb_(this, true))
            {
                return false;
            }
        }
        else if (currentState_ == State::Authorized)
        {
            authCb_(this, false);
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("Exception inside transitionTo: {}", e.what());
    }

    currentState_ = newState;
    return true;
}

} // namespace km
