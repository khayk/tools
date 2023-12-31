#include <kidmon/KidmonServer.h>
#include <kidmon/common/Utils.h>
#include <kidmon/os/Api.h>

#include <core/network/TcpServer.h>
#include <core/network/TcpCommunicator.h>
#include <core/utils/Str.h>

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

class AgentManager
{
    ServerMsgHandler& handler_;
    std::unique_ptr<tcp::Communicator> comm_;
    std::unordered_map<std::wstring, bool> users_;
    std::unordered_map<const tcp::Connection*, std::wstring> conns_;

    void updateMetadata(const tcp::Connection* conn, const bool connected)
    {
        if (connected)
        {
            const std::wstring username = sys::activeUserName();
            users_[username] = true;
            conns_[conn] = username;
        }
        else
        {
            users_[conns_[conn]] = false;
            conns_.erase(conn);
        }
    }

public:
    AgentManager(ServerMsgHandler& handler, tcp::Server& svr)
        : handler_(handler)
    {
        svr.onConnection([this](tcp::Connection& conn) {
            spdlog::info("Accepted: {}", fmt::ptr(&conn));
            comm_ = std::make_unique<tcp::Communicator>(conn);
            updateMetadata(&conn, true);

            comm_->onMsg([this](const std::string& req) {
                spdlog::trace("Server rcvd: {}", req);

                std::string resp;
                handler_.handle(req, resp);

                spdlog::trace("Server send: {}", resp);
                comm_->send(resp);
            });

            conn.onDisconnect([this, &conn]() {
                spdlog::info("Dropped: {}", fmt::ptr(&conn));
                updateMetadata(&conn, false);
            });

            conn.onError([this, &conn](const ErrorCode& ec) {
                spdlog::error("Connection error: {}, ec: {}, msg: {}",
                              fmt::ptr(&conn),
                              ec.value(),
                              ec.message());
                conn.close();
                updateMetadata(&conn, false);
            });

            comm_->start();
        });

        svr.onListening([](uint16_t port) {
            spdlog::trace("Listening on a port: {}", port);
        });

        svr.onClose([]() {
            spdlog::trace("Server closed");
        });

        svr.onError([](const ErrorCode& ec) {
            spdlog::error("Server error - ec: {}, msg: {}", ec.value(), ec.message());
        });
    }

    bool hasAgent(const std::wstring& username) const noexcept
    {
        const auto it = users_.find(username);

        return it != users_.end() && it->second;
    }
};

class KidmonServer::Impl
{
    using work_guard = net::executor_work_guard<net::io_context::executor_type>;
    using time_point = net::steady_timer::time_point;

    ServerMsgHandler handler_;
    std::unique_ptr<AgentManager> agentMngr_;
    net::io_context ioc_;
    tcp::Server svr_;
    net::steady_timer timer_;
    work_guard workGuard_;
    std::chrono::milliseconds timeout_;
    ApiPtr api_;
    ProcessLauncherPtr launcher_;
    bool spawnAgent_;

    bool agentRunning() const
    {
        return agentMngr_->hasAgent(sys::activeUserName());
    }

public:
    Impl(const Config& cfg)
        : ioc_()
        , svr_(ioc_)
        , timer_(ioc_)
        , workGuard_(ioc_.get_executor())
        , timeout_(cfg.activityCheckInterval)
        , api_(ApiFactory::create())
        , launcher_(api_->createProcessLauncher())
        , spawnAgent_(cfg.spawnAgent)
    {
        agentMngr_ = std::make_unique<AgentManager>(handler_, svr_);

        tcp::Server::Options opts;
        opts.port = cfg.serverPort;

        svr_.listen(opts);
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
