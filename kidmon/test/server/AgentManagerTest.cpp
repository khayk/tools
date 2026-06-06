#include <gtest/gtest.h>

#include <kidmon/server/AgentManager.h>
#include <kidmon/server/handler/AuthorizationHandler.h>
#include <kidmon/server/handler/DataHandler.h>
#include <kidmon/repo/FileSystemRepository.h>
#include <kidmon/data/Messages.h>

#include <core/network/TcpServer.h>
#include <core/network/TcpClient.h>
#include <core/network/TcpCommunicator.h>
#include <core/utils/File.h>
#include <core/utils/Str.h>
#include <core/utils/Sys.h>
#include <core/utils/Log.h>

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <optional>

using namespace km;
using namespace core::tcp;
using namespace std::chrono_literals;
using core::utl::MuteLogger;

namespace {

// Captures what a single agent-side client observed during the handshake.
struct ClientState
{
    explicit ClientState(IoContext& ioc)
        : client(ioc)
    {
    }

    Client client;
    std::unique_ptr<Communicator> comm;
    std::optional<bool> authorized; // answer.authorized from the server response
    bool gotResponse {false};
    bool disconnected {false};
};

} // namespace

// Stage 0 regression: once an agent is authorized, a second connection that
// presents the *same valid token* must NOT be granted authorization — it must
// be refused so it can never reach the data path (token-replay injection).
TEST(AgentManagerTest, SecondConcurrentAgentIsRejected)
{
    core::file::TempDir reportsDir("kdmn-tst");
    MuteLogger mute;

    IoContext ioc;
    Server svr(ioc);

    const std::string token = "secret-token";
    AuthorizationHandler authHandler;
    authHandler.setToken(token);

    FileSystemRepository repo(reportsDir.path());
    DataHandler dataHandler(repo);

    // Large peer-drop timeout so neither connection is dropped for inactivity
    // during the short test window.
    AgentManager mngr(authHandler, dataHandler, svr, 60s);

    Server::Options sopts;
    sopts.port = 51'123; // distinct from a possibly-running server (51097)
    svr.listen(sopts);

    const std::string username = core::str::ws2s(core::sys::activeUserName());
    const std::string authMsg = [&] {
        nlohmann::ordered_json js;
        msgs::buildAuthMsg(token, username, js);
        return js.dump();
    }();

    ClientState c1(ioc);
    ClientState c2(ioc);
    bool c2Started = false;

    const auto finishIfReady = [&]() {
        const bool c1Done = c1.authorized.has_value();
        const bool c2Done = c2.gotResponse || c2.disconnected;
        if (c1Done && c2Done)
        {
            ioc.stop();
        }
    };

    const auto parseAuthorized = [](const std::string& msg) -> std::optional<bool> {
        try
        {
            const auto js = nlohmann::json::parse(msg);
            if (js.contains("answer") && js["answer"].contains("authorized"))
            {
                return js["answer"]["authorized"].get<bool>();
            }
        }
        catch (const std::exception&)
        {
        }
        return std::nullopt;
    };

    const Client::Options copts {"127.0.0.1", sopts.port};

    const auto wire = [&](ClientState& c, const std::function<void()>& afterMsg) {
        c.client.onConnect([&, afterMsg](Connection& conn) {
            c.comm = std::make_unique<Communicator>(conn);
            c.comm->onMsg([&, afterMsg](const std::string& msg) {
                c.gotResponse = true;
                c.authorized = parseAuthorized(msg);
                afterMsg();
                finishIfReady();
            });
            conn.onDisconnect([&]() {
                c.disconnected = true;
                finishIfReady();
            });
            conn.onError([&](const ErrorCode&) {
                c.disconnected = true;
                finishIfReady();
            });
            c.comm->start();
            c.comm->sendAsync(authMsg);
        });
        c.client.onError([&](const ErrorCode&) {
            c.disconnected = true;
            finishIfReady();
        });
    };

    // c2 connects only after c1 is authorized, guaranteeing c1 is the
    // established agent and c2 is the contending second one.
    wire(c2, [] {});
    wire(c1, [&] {
        if (!c2Started)
        {
            c2Started = true;
            c2.client.connect(copts);
        }
    });

    c1.client.connect(copts);

    ioc.run_for(3s);

    // c1 is the one and only authorized agent.
    ASSERT_TRUE(c1.authorized.has_value());
    EXPECT_TRUE(*c1.authorized);
    EXPECT_FALSE(c1.disconnected);
    EXPECT_TRUE(mngr.hasAuthorizedAgent());

    // c2 must never be granted authorization: it is told authorized=false
    // and/or dropped, but it must not be treated as an authorized agent.
    EXPECT_FALSE(c2.authorized.value_or(false));
    EXPECT_TRUE(c2.disconnected || c2.authorized == std::optional<bool>(false));
}

// A heartbeat from an authorized agent must be accepted (keep-alive), not routed
// to the data handler -- otherwise it fails and the server drops the connection.
TEST(AgentManagerTest, HeartbeatFromAuthorizedAgentKeepsConnection)
{
    core::file::TempDir reportsDir("kdmn-tst");
    MuteLogger mute;

    IoContext ioc;
    Server svr(ioc);

    const std::string token = "secret-token";
    AuthorizationHandler authHandler;
    authHandler.setToken(token);

    FileSystemRepository repo(reportsDir.path());
    DataHandler dataHandler(repo);

    AgentManager mngr(authHandler, dataHandler, svr, 60s);

    Server::Options sopts;
    sopts.port = 51'124;
    svr.listen(sopts);

    const std::string username = core::str::ws2s(core::sys::activeUserName());
    const std::string authMsg = [&] {
        nlohmann::ordered_json js;
        msgs::buildAuthMsg(token, username, js);
        return js.dump();
    }();
    const std::string heartbeatMsg = [] {
        nlohmann::ordered_json js;
        msgs::buildHeartbeat(1000, 900, js);
        return js.dump();
    }();

    ClientState c(ioc);
    bool heartbeatSent = false;
    int responses = 0;

    c.client.onConnect([&](Connection& conn) {
        c.comm = std::make_unique<Communicator>(conn);
        c.comm->onMsg([&](const std::string&) {
            ++responses;
            // First response acknowledges auth; reply with a heartbeat. The
            // second response acknowledges that heartbeat -- we are done.
            if (!heartbeatSent)
            {
                heartbeatSent = true;
                c.comm->sendAsync(heartbeatMsg);
            }
            else
            {
                ioc.stop();
            }
        });
        conn.onDisconnect([&]() {
            c.disconnected = true;
            ioc.stop();
        });
        conn.onError([&](const ErrorCode&) {
            c.disconnected = true;
            ioc.stop();
        });
        c.comm->start();
        c.comm->sendAsync(authMsg);
    });
    c.client.onError([&](const ErrorCode&) {
        c.disconnected = true;
        ioc.stop();
    });

    c.client.connect({"127.0.0.1", sopts.port});
    ioc.run_for(3s);

    // The heartbeat was acknowledged, the connection survived, and the agent is
    // still the authorized one.
    EXPECT_TRUE(heartbeatSent);
    EXPECT_GE(responses, 2);
    EXPECT_FALSE(c.disconnected);
    EXPECT_TRUE(mngr.hasAuthorizedAgent());
}
