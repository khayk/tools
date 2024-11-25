#include <gtest/gtest.h>
#include <core/network/TcpClient.h>
#include <core/network/TcpServer.h>
#include <core/network/TcpCommunicator.h>

using namespace tcp;

namespace {

void asyncCommunicatorTest(const std::vector<std::string>& input)
{
    using namespace std::chrono_literals;

    IoContext ioc;
    Server svr(ioc);
    Server::Options sopts {1234};
    std::vector<std::string> received;

    uint16_t listeningPort = 0;
    svr.onListening([&listeningPort](uint16_t port) {
        listeningPort = port;
    });

    std::unique_ptr<Communicator> svrComm;
    svr.onConnection([&](Connection& conn) {
        svrComm = std::make_unique<tcp::Communicator>(conn);

        svrComm->onMsg([&received, &input, &conn](const std::string& msg) {
            received.push_back(msg);
            if (received.size() == input.size())
            {
                conn.close();
            }
        });

        conn.onDisconnect([&]() {
            ioc.stop();
        });

        conn.onError([&](const ErrorCode& ec) {
            std::ignore = ec;
            ioc.stop();
        });

        svrComm->start();
    });
    svr.listen(sopts);

    ASSERT_EQ(listeningPort, sopts.port);

    Client cnt(ioc);
    Client::Options copts {"127.0.0.1", sopts.port};
    std::unique_ptr<Communicator> clnComm;

    cnt.onConnect([&](Connection& conn) {
        clnComm = std::make_unique<tcp::Communicator>(conn);
        clnComm->onMsg([](const std::string& /* msg */) {});

        conn.onDisconnect([&]() {
            ioc.stop();
        });

        conn.onError([&](const ErrorCode& ec) {
            std::ignore = ec;
            ioc.stop();
        });

        clnComm->start();

        const auto cb = [](bool res) {
            EXPECT_TRUE(res);
        };

        for (const auto& in : input)
        {
            clnComm->sendAsync(in, cb);
        }
    });
    cnt.connect(copts);

    // This is a maximum time dedicated for the test, but
    // in reality, test will take a few milliseconds
    ioc.run_for(1s);

    ASSERT_EQ(input.size(), received.size());

    for (size_t i = 0; i < input.size(); ++i)
    {
        EXPECT_EQ(input[0], received[0]);
    }
}

} // namespace


TEST(TcpCommunicatorTest, BasicUsage)
{
    std::vector<std::string> input {std::string(100, '*'), std::string(321, '$')};
    asyncCommunicatorTest(input);
}


TEST(TcpCommunicatorTest, Data64Kb)
{
    std::vector<std::string> input;
    input.emplace_back(64 * 1024, '*');

    asyncCommunicatorTest(input);
}

TEST(TcpCommunicatorTest, Data500Kb)
{
    std::vector<std::string> input;
    size_t data = 512 * 1024;
    size_t chunk = 1024;

    while (data >= chunk)
    {
        input.emplace_back(chunk, '*');
        data -= chunk;
    }

    asyncCommunicatorTest(input);
}


TEST(TcpCommunicatorTest, Data20Mb)
{
    std::vector<std::string> input;
    input.emplace_back(20 * 1024 * 1024, '*');

    asyncCommunicatorTest(input);
}
