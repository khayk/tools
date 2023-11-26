#include "KidmonServer.h"
#include "os/Api.h"

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>

namespace net = boost::asio;

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
    ServerLogic svrLogic_;

public:
    Impl(const Config& cfg)
        : ioc_()
        , timer_(ioc_)
        , workGuard_(ioc_.get_executor())
        , timeout_(cfg.activityCheckInterval)
        , api_(ApiFactory::create())
        , launcher_(api_->createProcessLauncher())
        , svrLogic_(ioc_, cfg.serverPort)
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
            
            if (!svrLogic_.connected())
            {
                std::vector<std::string> args;
                args.push_back("dummy arg");
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
