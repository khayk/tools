#include "KidmonServer.h"
#include "common/Utils.h"
#include "os/Api.h"
#include "network/TcpServer.h"
#include "network/data/Unpacker.h"
#include <network/TcpCommunicator.h>
#include <utils/Str.h>

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace net = boost::asio;

namespace {


class ServerMsgHandler
{
public:
    void handle(const std::string& req, std::string& resp)
    {
        const auto js = nlohmann::json::parse(req);
        
        // @todo:hayk - verify auth info

        resp = R"({"authorized": true})";
    }
};

} // namespace 

class ConnectionAccepter
{
    tcp::Server svr_;
    std::unique_ptr<tcp::Communicator> comm_;
    ServerMsgHandler handler_;
    std::unordered_map<std::wstring, bool> users_;

public :
    ConnectionAccepter(net::io_context& ioc, uint16_t port)
        : svr_(ioc)
    {
        svr_.onConnection([this](tcp::Connection& conn) {
            spdlog::trace("Accepted: {}", reinterpret_cast<void*>(&conn));
            comm_ = std::make_unique<tcp::Communicator>(conn);

            comm_->onMsg([this](const std::string& req) {
                spdlog::trace("Server rcvd: {}", req);

                std::string resp;
                handler_.handle(req, resp);
                
                spdlog::trace("Server send: {}", resp);
                comm_->send(resp);
            });

            comm_->onDisconnect([this]() {
                spdlog::info("Dropped client: {}", reinterpret_cast<void*>(comm_.get()));
            });

            comm_->onError([&conn](const ErrorCode& ec) {
                spdlog::error("Connection error - ec: {}, msg: {}",
                              ec.value(),
                              ec.message());
                conn.close();
            });

            comm_->start();
        });

        svr_.onListening([](uint16_t port) {
            spdlog::trace("Listening on a port: {}", port);
        });

        svr_.onClose([]() {
            spdlog::trace("Server closed");
        });

        svr_.onError([](const ErrorCode& ec) {
            spdlog::error("Server error - ec: {}, msg: {}", ec.value(), ec.message());
        });

        tcp::Server::Options opts;
        opts.port = port;

        svr_.listen(opts);
    }

    bool connected(const std::wstring& username) const noexcept
    {
        const auto it = users_.find(username);

        return it != users_.end() && it->second;
    }
};

class KidmonServer::Impl
{
    using work_guard = net::executor_work_guard<net::io_context::executor_type>;
    using time_point = net::steady_timer::time_point;

    net::io_context ioc_;
    net::steady_timer timer_;
    work_guard workGuard_;
    std::chrono::milliseconds timeout_;
    ApiPtr api_;
    ProcessLauncherPtr launcher_;
    ConnectionAccepter accepter_;
    bool spawnAgent_;

    bool agentRunning()
    {
        return accepter_.connected(sys::activeUserName());
    }

public:
    Impl(const Config& cfg)
        : ioc_()
        , timer_(ioc_)
        , workGuard_(ioc_.get_executor())
        , timeout_(cfg.activityCheckInterval)
        , api_(ApiFactory::create())
        , launcher_(api_->createProcessLauncher())
        , accepter_(ioc_, cfg.serverPort)
        , spawnAgent_(cfg.spawnAgent)
    {
    }

    void run()
    {
        // Set to never expire
        timer_.expires_at(time_point::max());
        healthCheck();

        ioc_.run();
    }

    void shutdown() noexcept
    {
        ioc_.stop();
    }

    void healthCheck()
    {
        timer_.expires_after(timeout_);
        timer_.async_wait(std::bind(&Impl::healthCheck, this));

        try
        {
            spdlog::trace("Calls healthCheck");

            if (spawnAgent_ && !agentRunning())
            {
                const std::vector<std::string> args = {"--token", "987650", "--agent"};
                launcher_->launch("kidmon-app.exe", args);
            }
        }
        catch (const std::exception& e)
        {
            spdlog::error("Exception in healthCheck, desc: {}", e.what());
        }
    }
};

KidmonServer::KidmonServer(const Config& cfg)
    : impl_(std::make_unique<Impl>(cfg))
{
}

KidmonServer::~KidmonServer()
{
    impl_.reset();
}

void KidmonServer::run()
{
    spdlog::trace("Running KidmonServer application");

    impl_->run();
}

void KidmonServer::shutdown() noexcept
{
    spdlog::trace("Shutdown requested");

    impl_->shutdown();
}
