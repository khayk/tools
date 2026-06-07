#include <kidmon/server/KidmonServer.h>
#include <kidmon/server/handler/AuthorizationHandler.h>
#include <kidmon/server/handler/DataHandler.h>
#include <kidmon/repo/FileSystemRepository.h>
#include <kidmon/repo/AsyncRepository.h>
#include <kidmon/server/AgentManager.h>
#include <kidmon/os/Api.h>
#include <kidmon/common/Utils.h>
#include <kidmon/common/Defaults.h>
#include <kidmon/data/Constants.h>

#include <core/network/TcpServer.h>
#include <core/utils/Str.h>
#include <core/utils/Sys.h>
#include <core/utils/FmtExt.h>

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

#include <optional>

namespace net = boost::asio;

namespace km {

class KidmonServer::Impl
{
    using work_guard = net::executor_work_guard<net::io_context::executor_type>;
    using time_point = net::steady_timer::time_point;

    AuthorizationHandler authHandler_;
    FileSystemRepository repo_;
    // Decorates repo_ so blocking disk writes run on a worker thread instead of
    // the asio event-loop thread. Declared after repo_ and before dataHandler_
    // so it is constructed after its target and destroyed before it: the worker
    // flushes pending writes into repo_ while repo_ is still alive.
    AsyncRepository asyncRepo_;
    DataHandler dataHandler_;

    std::unique_ptr<AgentManager> agentMngr_;
    net::io_context ioc_;
    core::tcp::Server svr_;
    net::steady_timer timer_;
    work_guard workGuard_;
    std::chrono::milliseconds timeout_;
    ApiPtr api_;
    ProcessLauncherPtr launcher_;
    bool spawnAgent_;

    // When set, an agent was spawned at this time but has not yet authorized.
    // Used to suppress re-spawning during a slow agent start (see healthCheck).
    // Only ever touched on the asio thread, so it needs no synchronization.
    std::optional<time_point> spawnedAt_;

    // How long to wait for a freshly spawned agent to connect and authorize
    // before assuming it failed and spawning a replacement. Comfortably exceeds
    // peerDropTimeout (activityCheckInterval + 2s) so a connected-but-unauthed
    // agent is dropped by the server before we ever consider re-spawning.
    std::chrono::milliseconds spawnGracePeriod_;

    [[nodiscard]] bool agentRunning() const
    {
        return agentMngr_->hasAuthorizedAgent();
    }

    // True while a previously spawned agent is still within its startup grace
    // window, i.e. we should not spawn another one yet.
    [[nodiscard]] bool spawnPending() const
    {
        return spawnedAt_.has_value() &&
               (time_point::clock::now() - *spawnedAt_) < spawnGracePeriod_;
    }

public:
    explicit Impl(const Config& cfg)
        : repo_(cfg.reportsDir)
        , asyncRepo_(repo_)
        , dataHandler_(asyncRepo_)
        , svr_(ioc_)
        , timer_(ioc_)
        , workGuard_(ioc_.get_executor())
        , timeout_(cfg.activityCheckInterval)
        , api_(ApiFactory::create())
        , launcher_(api_->createProcessLauncher())
        , spawnAgent_(cfg.spawnAgent)
        , spawnGracePeriod_(cfg.activityCheckInterval * 5)
    {
        spdlog::trace("Report dir: {}", cfg.reportsDir);

        authHandler_.setToken(cfg.authToken);
        agentMngr_ = std::make_unique<AgentManager>(authHandler_,
                                                    dataHandler_,
                                                    svr_,
                                                    cfg.peerDropTimeout);

        core::tcp::Server::Options opts;
        opts.port = cfg.listenPort;

        svr_.onError([&](const ErrorCode& ec) {
            spdlog::error("Server error - ec: {}, msg: {}", ec.value(), ec.message());
            ioc_.stop();
        });

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
        timer_.async_wait([this](const boost::system::error_code& ec) {
            if (ec)
            {
                return; // Don't run healthCheck if timer failed/was cancelled
            }

            this->healthCheck();
        });

        try
        {
            spdlog::trace("healthCheck");

            if (agentRunning())
            {
                // An agent is established; clear any pending-spawn marker so that
                // if it later dies we respawn promptly instead of waiting out a
                // stale grace window.
                spawnedAt_.reset();
            }
            else if (spawnAgent_ && !spawnPending())
            {
                // No authorized agent and none currently starting up. Spawn one
                // and remember when, so the next ticks don't spawn duplicates
                // (each with a fresh token that would invalidate the previous
                // agent's) while this one is still authorizing.
                const auto token = utl::generateToken(16);
                const Args args = {"--agent"};

                // Pass the token via the environment, not argv, so it is not
                // visible to other users in the process list.
                const Env env = {{std::string(constants::ENV_AUTH_TOKEN), token}};
                authHandler_.setToken(token);

                launcher_->launch(core::sys::currentProcessPath(), args, env);
                spawnedAt_ = time_point::clock::now();
            }
        }
        catch (const std::exception& e)
        {
            spdlog::error("Exception in healthCheck, desc: {}", e.what());
        }
    }
};

KidmonServer::Config::Config(const fs::path& appDataDir)
    : reportsDir {appDataDir / "reports"}
    , activityCheckInterval {defaults::ACTIVITY_CHECK_INTERVAL}
    , peerDropTimeout {activityCheckInterval + defaults::PEER_DROP_GRACE}
    , listenPort {defaults::SERVER_PORT}
    , spawnAgent {true}
{
}

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

} // namespace km
