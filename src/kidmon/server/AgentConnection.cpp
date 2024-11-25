#include <kidmon/server/AgentConnection.h>
#include <kidmon/server/handler/AuthorizationHandler.h>
#include <kidmon/server/handler/DataHandler.h>
#include <kidmon/data/Messages.h>
#include <core/utils/Str.h>
#include <core/utils/Sys.h>

#include <boost/asio/error.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

AgentConnection::AgentConnection(AuthorizationHandler& authHandler,
                                 DataHandler& dataHandler,
                                 tcp::Socket&& sock,
                                 std::chrono::milliseconds peerDropTimeout)
    : Connection(std::move(sock))
    , authHandler_(authHandler)
    , dataHandler_(dataHandler)
    , comm_(*this)
{
    comm_.onMsg([this](const std::string& msg) {
        try
        {
            spdlog::debug("Server rcvd: {} bytes", msg.size());

            const auto payload = nlohmann::json::parse(msg);
            nlohmann::json answer;
            std::string error;

            if (currentState_ == State::Authorized)
            {
                dataHandler_.handle(payload, answer, error);
            }
            else if (authHandler_.handle(payload, answer, error))
            {
                transitionTo(State::Authorized);
            }

            if (!error.empty())
            {
                spdlog::error("Request handling failed, details: {}", error);

                // This means unknown or bad message
                transitionTo(State::Disconnected);
                close();
                return;
            }

            nlohmann::ordered_json js;
            msgs::buildResponse(0, answer, js);
            const auto res = js.dump();
            spdlog::debug("Server sent: {} bytes", res.size());
            comm_.sendAsync(res);
        }
        catch (const std::exception& ex)
        {
            spdlog::error("Excepting in request handling, details: {}", ex.what());
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

        const auto activeUsername = str::ws2s(sys::activeUserName());

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

tcp::Communicator& AgentConnection::communicator()
{
    return comm_;
}

void AgentConnection::transitionTo(const State newState) noexcept
{
    if (currentState_ == State::Connected)
    {
        if (newState == State::Authorized)
        {
            authCb_(this, true);
        }
    }
    else if (currentState_ == State::Authorized)
    {
        authCb_(this, false);
    }

    currentState_ = newState;
}