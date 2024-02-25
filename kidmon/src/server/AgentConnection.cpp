#include <kidmon/server/AgentConnection.h>
#include <kidmon/server/handler/AuthorizationHandler.h>
#include <kidmon/server/handler/DataHandler.h>
#include <kidmon/data/Messages.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

AgentConnection::AgentConnection(AuthorizationHandler& authHandler,
                                 DataHandler& dataHandler,
                                 tcp::Socket&& socket)
    : Connection(std::move(socket))
    , authHandler_(authHandler)
    , dataHandler_(dataHandler)
    , comm_(*this)
{
    comm_.onMsg([this](const std::string& msg) {
        //spdlog::trace("Server rcvd: {}", msg);

        const auto payload = nlohmann::json::parse(msg);
        nlohmann::json answer;
        std::string error;

        if (status_ == Status::Authorized)
        {
            dataHandler_.handle(payload, answer, error);
        }
        else if (authHandler_.handle(payload, answer, error))
        {
            transitionTo(Status::Authorized);
        }

        if (!error.empty())
        {
            spdlog::error("Request handling failed, details: {}", error);

            // This means unknown or bad message
            transitionTo(Status::Disconnected);
            close();
            return;
        }

        nlohmann::ordered_json js;
        msgs::buildAnswer(0, answer, js);
        const auto res = js.dump();
        spdlog::trace("Server answers: {}", res);
        comm_.sendAsync(res);
    });

    onError([this](const ErrorCode& ec) {
        spdlog::error("Connection error: {}, ec: {}, msg: {}",
                      fmt::ptr(this),
                      ec.value(),
                      ec.message());
        transitionTo(Status::Disconnected);
        close();
    });
}

AgentConnection::~AgentConnection()
{
    spdlog::info("Dropped: {}", fmt::ptr(this));

    transitionTo(Status::Disconnected);
}

void AgentConnection::onAuth(AuthorizationCb authCb)
{
    authCb_ = std::move(authCb);
}

AgentConnection::Status AgentConnection::status() const noexcept
{
    return status_;
}

tcp::Communicator& AgentConnection::communicator()
{
    return comm_;
}

void AgentConnection::transitionTo(const Status status) noexcept
{
    if (status_ == Status::Connected)
    {
        if (status == Status::Authorized)
        {
            authCb_(this, true);
        }
    }
    else if (status_ == Status::Authorized)
    {
        authCb_(this, false);
    }

    status_ = status;
}