#include <gtest/gtest.h>
#include "network/TcpServer.h"
#include "network/TcpClient.h"

namespace {

using namespace tcp;

TEST(TcpServerTest, ListenAndClose)
{
    IoContext ioc;

    Server svr(ioc);
    Server::Options opts;
    opts.port = 7453;
    uint16_t actualPort {0};

    svr.onListening([&actualPort](uint16_t port) {
        actualPort = port;
    });

    svr.listen(opts);
    EXPECT_EQ(actualPort, opts.port);

    bool closeInvoked {false};
    svr.onClose([&closeInvoked]() {
        closeInvoked = true;
    });

    svr.close();
    EXPECT_EQ(closeInvoked, true);
}


TEST(TcpServerTest, ListenFails)
{
    IoContext ioc;

    Server svr(ioc);
    Server::Options opts;
    opts.port = 7453;

    svr.listen(opts);

    Server svr2(ioc);
    bool secondListenSucceeded = false;
    svr2.onListening([&secondListenSucceeded](uint16_t) {
        secondListenSucceeded = true;
    });

    ErrorCode ec2;
    svr2.onError([&ec2](const ErrorCode& ec) {
        ec2 = ec;
    });

    svr2.listen(opts);
    EXPECT_FALSE(secondListenSucceeded);
    EXPECT_TRUE((ec2));
}


TEST(TcpServerTest, AcceptConnection)
{
    using namespace std::chrono_literals;

    IoContext ioc;
    Server svr(ioc);
    Server::Options opts;
    opts.port = 7453;
    bool accepted = false;

    svr.onConnection([&accepted, &svr](Connection& conn) {
        accepted = true;
        conn.close();
        svr.close();
    });

    svr.listen(opts);

    Client cnt(ioc);
    Client::Options copts;
    copts.host = "127.0.0.1";
    copts.port = opts.port;

    cnt.connect(copts);
    ioc.run_for(1s);

    EXPECT_EQ(accepted, true);
}

} // namespace