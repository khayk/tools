#include <gtest/gtest.h>

#include "support/Helpers.h"

#include <kidmon/agent/KidmonAgent.h>
#include <kidmon/data/Messages.h>

#include <core/network/TcpServer.h>
#include <core/network/TcpCommunicator.h>
#include <core/utils/Log.h>

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>

using namespace km;
using namespace km::test;
using namespace core::tcp;
using core::utl::MuteLogger;

namespace {

// A fake server standing in for KidmonServer: it accepts the agent's connection,
// captures the auth message the agent sends, and replies with a caller-supplied
// response. This lets us exercise the agent's handshake state machine without an
// OS-dependent data-collection path.
class FakeServer
{
public:
    // Given the parsed auth message, produces the raw response to send back.
    using Responder = std::function<std::string(const nlohmann::json&)>;

    FakeServer(IoContext& ioc, uint16_t port, Responder responder)
        : svr_(ioc)
        , responder_(std::move(responder))
    {
        svr_.onConnection([this](Connection& conn) {
            comm_ = std::make_unique<Communicator>(conn);
            comm_->onMsg([this](const std::string& msg) {
                onAgentMsg(msg);
            });
            conn.onDisconnect([this]() {
                agentDisconnected_ = true;
            });
            conn.onError([this](const ErrorCode&) {
                agentDisconnected_ = true;
            });
            comm_->start();
        });

        Server::Options opts;
        opts.port = port;
        svr_.listen(opts);
    }

    [[nodiscard]] bool gotAuth() const noexcept
    {
        return gotAuth_;
    }

    [[nodiscard]] const nlohmann::json& authMsg() const noexcept
    {
        return authMsg_;
    }

    [[nodiscard]] bool agentDisconnected() const noexcept
    {
        return agentDisconnected_;
    }

private:
    void onAgentMsg(const std::string& msg)
    {
        // Only the first message (the auth handshake) is of interest; any
        // follow-up activity/heartbeat traffic is ignored by the fake server.
        if (gotAuth_)
        {
            return;
        }
        gotAuth_ = true;
        try
        {
            authMsg_ = nlohmann::json::parse(msg);
        }
        catch (const std::exception&)
        {
        }
        comm_->sendAsync(responder_(authMsg_));
    }

    Server svr_;
    Responder responder_;
    std::unique_ptr<Communicator> comm_;
    nlohmann::json authMsg_;
    bool gotAuth_ {false};
    bool agentDisconnected_ {false};
};

KidmonAgent::Config makeConfig(uint16_t port, std::string token)
{
    KidmonAgent::Config cfg;
    cfg.serverPort = port;
    cfg.authToken = std::move(token);
    cfg.takeSnapshots = false;
    cfg.calcSha = false;
    // Run the activity loop quickly so the authorized branch is exercised within
    // the test window.
    cfg.activityCheckInterval = 100ms;
    return cfg;
}

std::string approveResponse(bool authorized)
{
    nlohmann::ordered_json js;
    nlohmann::json answer = {{"authorized", authorized}};
    msgs::buildResponse(0, answer, js);
    return js.dump();
}

std::string errorResponse(int status, std::string_view error)
{
    nlohmann::ordered_json js;
    msgs::buildResponse(status, error, nlohmann::json::object(), js);
    return js.dump();
}

} // namespace

// The agent sends a well-formed auth message (correct token + active username);
// when the server denies authorization (authorized=false) the agent exits, so
// run() returns rather than hanging.
TEST(KidmonAgentTest, SendsWellFormedAuthAndExitsWhenDenied)
{
    MuteLogger mute;

    const uint16_t port = 51'141;
    const std::string token = "agent-token";

    IoContext ioc;
    FakeServer fake(ioc, port, [](const nlohmann::json&) {
        return approveResponse(false);
    });

    KidmonAgent agent(makeConfig(port, token));
    RunnableThread runner(agent);

    pumpUntil(
        ioc,
        [&] {
            return runner.done();
        },
        5s);
    runner.stopAndJoin();

    EXPECT_TRUE(runner.done());
    ASSERT_TRUE(fake.gotAuth());

    const auto& js = fake.authMsg();
    EXPECT_EQ(js.value("name", ""), "auth");
    ASSERT_TRUE(js.contains("message"));
    EXPECT_EQ(js["message"].value("token", ""), token);
    EXPECT_EQ(js["message"].value("username", ""), activeUser());
}

// A non-zero status during the auth handshake means the server rejected the
// agent (bad token, an agent already connected, ...). The agent must shut down
// deterministically instead of staying connected.
TEST(KidmonAgentTest, ExitsWhenServerReportsAuthError)
{
    MuteLogger mute;

    const uint16_t port = 51'142;

    IoContext ioc;
    FakeServer fake(ioc, port, [](const nlohmann::json&) {
        return errorResponse(1, "bad token");
    });

    KidmonAgent agent(makeConfig(port, "agent-token"));
    RunnableThread runner(agent);

    pumpUntil(
        ioc,
        [&] {
            return runner.done();
        },
        5s);
    runner.stopAndJoin();

    EXPECT_TRUE(runner.done());
    EXPECT_TRUE(fake.gotAuth());
}

// When authorization succeeds the agent must transition into its activity loop
// and keep the connection -- it must not exit or drop the socket. (The activity
// data itself is OS-dependent and therefore not asserted here.)
TEST(KidmonAgentTest, StaysConnectedWhenAuthorized)
{
    MuteLogger mute;

    const uint16_t port = 51'143;

    IoContext ioc;
    FakeServer fake(ioc, port, [](const nlohmann::json&) {
        return approveResponse(true);
    });

    KidmonAgent agent(makeConfig(port, "agent-token"));
    RunnableThread runner(agent);

    // The agent never exits on its own once authorized, so this runs to the
    // deadline.
    pumpUntil(
        ioc,
        [&] {
            return runner.done();
        },
        1s);

    EXPECT_FALSE(runner.done()); // still running its activity loop
    EXPECT_TRUE(fake.gotAuth());
    EXPECT_FALSE(fake.agentDisconnected()); // connection kept alive
}
