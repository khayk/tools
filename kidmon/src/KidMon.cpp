#include "KidMon.h"
#include "os/Api.h"

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include <iomanip>
#include <sstream>
#include <thread>

namespace net = boost::asio;

std::ostream& operator<<(std::ostream& os, const Point& pt)
{
    os << '(' << pt.x() << ", " << pt.y() << ')';

    return os;
}

std::ostream& operator<<(std::ostream& os, const Rect& rc)
{
    os << '[' << rc.leftTop() << ", " << rc.rightBottom() << ']';

    return os;
}

class KidMon::Impl
{
    using work_guard = net::executor_work_guard<net::io_context::executor_type>;
    using clock_type = net::steady_timer::clock_type;
    using time_point = net::steady_timer::time_point;

    net::io_context ioc_;
    net::steady_timer timer_;
    work_guard workGuard_;
    std::chrono::milliseconds timeoutMs_;

    ApiPtr api_;

public:
    Impl()
        : ioc_()
        , timer_(ioc_)
        , workGuard_(ioc_.get_executor())
        , timeoutMs_(1000)
        , api_(ApiFactory::create())
    {
    }

    void run()
    {
        using namespace std::chrono_literals;

        // Set to never expire
        timer_.expires_at(time_point::max());

        collectData();

        ioc_.run();
    }

    void shutdown() noexcept
    {
        ioc_.stop();
    }

    void collectData()
    {
        timer_.expires_after(timeoutMs_);
        timer_.async_wait(std::bind(&Impl::collectData, this));

        try
        {
            spdlog::trace("Calls collectData");

            auto window = api_->forgroundWindow();
            if (!window)
            {
                spdlog::warn("Unable to detect forground window");
                return;
            }

            spdlog::trace("{}, '{:64}', {:32}",
                window->id(),
                window->title(),
                window->className());

            Rect rc = window->boundingRect();

            std::ostringstream oss;
            oss << rc;

            spdlog::trace("Forground wnd {}", oss.str());
            spdlog::trace("Executable: {}\n", window->ownerProcessPath());
        }
        catch (const std::exception& e)
        {
            spdlog::error("Failed to cleanup broken enviornment. Desc: {}", e.what());
        }
    }
};

KidMon::KidMon()
    : impl_(std::make_unique<Impl>())
{
}

KidMon::~KidMon()
{
    impl_.reset();
}

void KidMon::run()
{
    spdlog::trace("Running KidMon application");

    impl_->run();
}

void KidMon::shutdown() noexcept
{
    spdlog::trace("Shutdown requested");

    impl_->shutdown();
}
