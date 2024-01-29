#include <kidmon/server/AgentConnection.h>
#include <kidmon/server/handler/AuthorizationHandler.h>
#include <kidmon/server/handler/DataHandler.h>

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
    // if (authHandler_.hasAuthorizedAgent())
    // {
    //     comm_.send(R"({"error": "Already has an authorized agent"})");
    //     close();
    //     return;
    // }

    comm_.onMsg([this](const std::string& msg) {
        spdlog::trace("Server rcvd: {}", msg);

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

        const nlohmann::ordered_json js = {
            {"status",0},
            {"error", ""},
            {"answer", answer}
        };

        const auto res = js.dump();
        spdlog::trace("Server send: {}", res);
        comm_.sendAsync(res, [](bool) {});
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