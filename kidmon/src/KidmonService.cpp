#include "KidmonService.h"
#include "os/Api.h"

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>

namespace net = boost::asio;

class KidmonService::Impl
{
    using work_guard = net::executor_work_guard<net::io_context::executor_type>;
    using time_point = net::steady_timer::time_point;

    net::io_context ioc_;
    net::steady_timer timer_;
    work_guard workGuard_;
    std::chrono::milliseconds timeout_;
    ApiPtr api_;

public:
    Impl(const Config& cfg)
        : ioc_()
        , timer_(ioc_)
        , workGuard_(ioc_.get_executor())
        , timeout_(cfg.activityCheckInterval)
        , api_(ApiFactory::create())
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
            auto launcher = api_->createProcessLauncher();
            
            std::vector<std::string> args;
            args.push_back("dummy arg");
            launcher->launch("kidmon-app.exe", args);
        }
        catch (const std::exception& e)
        {
            spdlog::error("Exception in healthCheck, desc: {}", e.what());
        }
    }
};

KidmonService::KidmonService(const Config& cfg)
    : impl_(std::make_unique<Impl>(cfg))
{
}

KidmonService::~KidmonService()
{
    impl_.reset();
}

void KidmonService::run()
{
    spdlog::trace("Running KidmonService application");

    impl_->run();
}

void KidmonService::shutdown() noexcept
{
    spdlog::trace("Shutdown requested");

    impl_->shutdown();
}
