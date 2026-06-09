#include <gtest/gtest.h>

#include "support/Helpers.h"

#include <kidmon/server/KidmonServer.h>

#include <core/network/TcpClient.h>
#include <core/network/TcpCommunicator.h>
#include <core/utils/File.h>
#include <core/utils/Log.h>

#include <filesystem>
#include <memory>
#include <optional>

using namespace km;
using namespace km::test;
using namespace core::tcp;
using core::utl::MuteLogger;

namespace {

namespace fs = std::filesystem;

// KidmonServer starts listening from inside its constructor and runs its own
// io_context on a background thread. This fixture owns that thread and the
// lifetime so each test can drive a fake agent against a real server instance.
//
// Note: tests always use spawnAgent=false. The health-check spawn path launches
// the current executable with "--agent", which in a test binary would fork the
// test runner itself -- it is not exercisable without a process-launcher seam
// (the server constructs its launcher internally via ApiFactory).
class ServerFixture
{
public:
    ServerFixture(uint16_t port, std::string token)
        : reportsDir_("kdmn-srv-tst")
    {
        KidmonServer::Config cfg(reportsDir_.path());
        cfg.listenPort = port;
        cfg.authToken = std::move(token);
        cfg.spawnAgent = false;
        // Keep the health-check timer quiet; with spawnAgent=false it is a no-op.
        cfg.activityCheckInterval = 1s;

        server_ = std::make_unique<KidmonServer>(cfg);
        runner_.emplace(*server_);
    }

    // Stops the server, joins its thread, and destroys the instance so the
    // AsyncRepository worker flushes every pending write before we inspect disk.
    void stop()
    {
        runner_.reset();
        server_.reset();
    }

    [[nodiscard]] std::size_t persistedEntryCount() const
    {
        std::size_t count = 0;
        if (!fs::exists(reportsDir_.path()))
        {
            return count;
        }
        for (const auto& it : fs::recursive_directory_iterator(reportsDir_.path()))
        {
            // Raw entries are appended to files under a per-user "raw" dir;
            // snapshots (none in these tests) would land under "snapshots".
            if (it.is_regular_file() && it.path().parent_path().filename() == "raw")
            {
                ++count;
            }
        }
        return count;
    }

private:
    core::file::TempDir reportsDir_;
    std::unique_ptr<KidmonServer> server_;
    std::optional<RunnableThread> runner_;
};

// A fake agent: connects, sends one auth message, then optionally a data message
// once authorized, recording the server's responses.
struct FakeAgent
{
    explicit FakeAgent(IoContext& ioc)
        : client(ioc)
    {
    }

    Client client;
    std::unique_ptr<Communicator> comm;
    std::optional<bool> authorized;
    int responses {0};
    bool disconnected {false};
};

} // namespace

// End-to-end happy path: a valid agent authorizes, sends one activity entry, and
// the server persists it through its AsyncRepository to the reports directory.
TEST(KidmonServerTest, AcceptsAuthorizedAgentAndPersistsData)
{
    MuteLogger mute;

    const uint16_t port = 51'131;
    const std::string token = "server-token";
    ServerFixture server(port, token);

    const std::string username = activeUser();

    IoContext ioc;
    FakeAgent agent(ioc);
    bool dataSent = false;

    agent.client.onConnect([&](Connection& conn) {
        agent.comm = std::make_unique<Communicator>(conn);
        agent.comm->onMsg([&](const std::string& msg) {
            ++agent.responses;
            if (!dataSent)
            {
                // First response acknowledges auth; capture it and send data.
                agent.authorized = parseAuthorized(msg);
                dataSent = true;
                agent.comm->sendAsync(makeDataMsg(sampleEntry(username)));
            }
            else
            {
                // Second response acknowledges the data message -- done.
                ioc.stop();
            }
        });
        conn.onDisconnect([&]() {
            agent.disconnected = true;
            ioc.stop();
        });
        conn.onError([&](const ErrorCode&) {
            agent.disconnected = true;
            ioc.stop();
        });
        agent.comm->start();
        agent.comm->sendAsync(makeAuthMsg(token, username));
    });
    agent.client.onError([&](const ErrorCode&) {
        agent.disconnected = true;
        ioc.stop();
    });

    agent.client.connect({"127.0.0.1", port});
    ioc.run_for(3s);

    EXPECT_TRUE(agent.authorized.value_or(false));
    EXPECT_GE(agent.responses, 2);
    EXPECT_FALSE(agent.disconnected);

    // Destroy the server so the async write is flushed, then assert it landed.
    server.stop();
    EXPECT_EQ(server.persistedEntryCount(), 1U);
}

// An agent presenting the wrong token must not be authorized and must not be
// able to persist anything.
TEST(KidmonServerTest, RejectsAgentWithInvalidToken)
{
    MuteLogger mute;

    const uint16_t port = 51'132;
    ServerFixture server(port, "correct-token");

    const std::string username = activeUser();

    IoContext ioc;
    FakeAgent agent(ioc);

    agent.client.onConnect([&](Connection& conn) {
        agent.comm = std::make_unique<Communicator>(conn);
        agent.comm->onMsg([&](const std::string& msg) {
            ++agent.responses;
            agent.authorized = parseAuthorized(msg);
            ioc.stop();
        });
        conn.onDisconnect([&]() {
            agent.disconnected = true;
            ioc.stop();
        });
        conn.onError([&](const ErrorCode&) {
            agent.disconnected = true;
            ioc.stop();
        });
        agent.comm->start();
        agent.comm->sendAsync(makeAuthMsg("wrong-token", username));
    });
    agent.client.onError([&](const ErrorCode&) {
        agent.disconnected = true;
        ioc.stop();
    });

    agent.client.connect({"127.0.0.1", port});
    ioc.run_for(3s);

    // The agent is told it is not authorized (and/or dropped); never authorized.
    EXPECT_FALSE(agent.authorized.value_or(false));

    server.stop();
    EXPECT_EQ(server.persistedEntryCount(), 0U);
}
