#include <kidmon/server/KidmonServer.h>
#include <kidmon/server/handler/AuthorizationHandler.h>
#include <kidmon/server/handler/DataHandler.h>
#include <kidmon/repo/FileSystemRepository.h>
#include <kidmon/server/AgentManager.h>
#include <kidmon/os/Api.h>
#include <kidmon/config/Config.h>
#include <kidmon/common/Utils.h>

#include <core/network/TcpServer.h>
#include <core/utils/Str.h>
#include <core/utils/Sys.h>

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace net = boost::asio;

class KidmonServer::Impl
{
    using work_guard = net::executor_work_guard<net::io_context::executor_type>;
    using time_point = net::steady_timer::time_point;

    AuthorizationHandler authHandler_;
    FileSystemRepository repo_;
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
        : repo_(cfg.reportsDir)
        , dataHandler_(repo_)
        , svr_(ioc_)
        , timer_(ioc_)
        , workGuard_(ioc_.get_executor())
        , timeout_(cfg.activityCheckInterval)
        , api_(ApiFactory::create())
        , launcher_(api_->createProcessLauncher())
        , spawnAgent_(cfg.spawnAgent)
    {
        authHandler_.setToken(cfg.authToken);
        agentMngr_ = std::make_unique<AgentManager>(authHandler_,
                                                    dataHandler_,
                                                    svr_,
                                                    cfg.peerDropTimeout);

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
            spdlog::trace("healthCheck");

            if (spawnAgent_ && !agentRunning())
            {
                const auto token = utl::generateToken(16);
                const std::vector<std::string> args = {"--token", token, "--agent"};
                authHandler_.setToken(token);

                launcher_->launch(sys::currentProcessPath(), args);
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
    spdlog::info("Running KidmonServer");

    impl_->run();
}

void KidmonServer::shutdown() noexcept
{
    spdlog::info("Shutdown requested");

    impl_->shutdown();
}
