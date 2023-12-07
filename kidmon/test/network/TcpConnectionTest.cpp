#include <gtest/gtest.h>
#include "network/TcpClient.h"
#include "network/TcpServer.h"

namespace {

using namespace tcp;

struct Manager
{
    Connection& conn;
    const std::string& sent;
    size_t off {0};
    const size_t rcvdMax;
    
public:
    std::string rcvd;
    ErrorCode errc {};
    bool disconnected {false};

    Manager(Connection& con, const std::string& outgoing, size_t rcvMax)
        : conn(con)
        , sent(outgoing)
        , rcvdMax(rcvMax)
    {
        rcvd.reserve(rcvdMax);

        conn.onRead([this](const char* data, size_t size) {
            rcvd.append(data, size);
            
            // Stop reading if there is no more data is coming
            if (rcvd.size() < rcvdMax)
            {
                conn.read();
            }
            else if (sent.size() == off)
            {
                conn.close();
            }
        });
        conn.onSent([this](size_t size) {
            off += size;

            if (off < sent.size())
            {
                conn.write(sent.data() + off, std::min<size_t>(4096, sent.size() - off));
            }
            else if (rcvd.size() >= rcvdMax)
            {
                conn.close();
            }
        });
        conn.onError([this](const ErrorCode& ec) {
            errc = ec;
            conn.close();
        });
        conn.onDisconnect([this]() {
            disconnected = true;
        });
        conn.read();
        conn.write(sent.data(), 1);
    }
};

TEST(TcpConnectionTest, DataTransfer)
{
    using namespace std::chrono_literals;
    
    const std::string clientToSend = "hello from client";
    const std::string serverToSend = "greetings from server!";
    std::unique_ptr<Manager> serverMngr;
    std::unique_ptr<Manager> clientMngr;

    IoContext ioc;
    Server svr(ioc);
    Server::Options sopts {1234};

    svr.onConnection([&](Connection& conn) {
        serverMngr = std::make_unique<Manager>(conn, serverToSend, clientToSend.size());
        conn.onDisconnect([&]() {
            svr.close();
        });
    });
    svr.listen(sopts);

    Client cnt(ioc);
    Client::Options copts {"127.0.0.1", sopts.port};

    cnt.onConnect([&](Connection& conn) {
        clientMngr = std::make_unique<Manager>(conn, clientToSend, serverToSend.size());
    });
    cnt.connect(copts);

    // This is a maxiumum time dedicated for the tast, but 
    // in reality, test will take a few milliseconds
    ioc.run_for(2s);

    ASSERT_TRUE(serverMngr);   
    EXPECT_EQ(serverMngr->rcvd, clientToSend);
    EXPECT_TRUE(serverMngr->disconnected);
    EXPECT_TRUE(!(serverMngr->errc));

    ASSERT_TRUE(clientMngr);
    EXPECT_EQ(clientMngr->rcvd, serverToSend);
    EXPECT_TRUE(!(clientMngr->errc));
    EXPECT_TRUE(clientMngr->disconnected);
}

} // namespace