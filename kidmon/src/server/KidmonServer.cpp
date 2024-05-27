#include <kidmon/server/KidmonServer.h>
#include <kidmon/server/handler/AuthorizationHandler.h>
#include <kidmon/server/handler/DataHandler.h>
#include <kidmon/server/AgentManager.h>
#include <kidmon/os/Api.h>

#include <kidmon/config/Config.h>
#include <kidmon/common/Utils.h>

#include <core/utils/Str.h>
#include <core/network/TcpServer.h>

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace net = boost::asio;

class KidmonServer::Impl
{
    using work_guard = net::executor_work_guard<net::io_context::executor_type>;
    using time_point = net::steady_timer::time_point;

    AuthorizationHandler authHandler_;
    FileSystemStorage storage_;
    DataHandler dataHandler_;

    std::unique_ptr<AgentManager> agentMngr_;
    net::io_context ioc_;
    tcp::Server svr_;
    net::steady_timer timer_;
    work_guard workGuard_;
    std::chrono::milliseconds timeout_;
    ApiPtr api_;
    ProcessLauncherPtr launcher_;
    bool spawnAgent_;

    [[nodiscard]] bool agentRunning() const
    {
        return agentMngr_->hasAuthorizedAgent();
    }

public:
    explicit Impl(const Config& cfg)
        : storage_(cfg.reportsDir)
        , dataHandler_(storage_)
        , svr_(ioc_)
        , timer_(ioc_)
        , workGuard_(ioc_.get_executor())
        , timeout_(cfg.activityCheckInterval)
        , api_(ApiFactory::create())
        , launcher_(api_->createProcessLauncher())
        , spawnAgent_(cfg.spawnAgent)
    {
        agentMngr_ = std::make_unique<AgentManager>(authHandler_, dataHandler_, svr_);

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
                const auto token = utl::generateToken(16);
                const std::vector<std::string> args = {"--token", token, "--agent"};
                authHandler_.setToken(token);

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
    spdlog::trace("Running KidmonServer");

    impl_->run();
}

void KidmonServer::shutdown() noexcept
{
    spdlog::trace("Shutdown requested");

    impl_->shutdown();
}
