#include "KidMon.h"
#include "os/Api.h"
#include "common/Utils.h"

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

namespace net = boost::asio;
namespace fs = std::filesystem;

using TimePoint = std::chrono::system_clock::time_point;

struct ProcessInfo
{
    std::wstring path;
    std::string sha256;
};

struct WindowInfo
{
    std::string snapshotPath;
    std::string title;
    Rect placement;
};

struct Entry
{
    ProcessInfo procssInfo;
    WindowInfo windowInfo;
    TimePoint timestamp;
};

struct ReportDirs
{
    std::string snapshotsDir;
    std::string dailyDir;
    std::string monthlyDir;
    std::string weeklyDir;
    std::string rawDir;
};

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

    const Config& cfg_;

    net::io_context ioc_;
    net::steady_timer timer_;
    work_guard workGuard_;
    std::chrono::milliseconds timeoutMs_;

    ApiPtr api_;
    std::vector<char> wndContent_;
    size_t index_{ 0 };

    std::unordered_map<std::wstring, ReportDirs> dirs_;

    const ReportDirs& getActiveUserDirs()
    {
        std::wstring activeUserName = SysUtils::activeUserName();

        if (auto it = dirs_.find(activeUserName); it != dirs_.end())
        {
            return it->second;
        }

        std::time_t t = std::time(0);   // get time now
        std::tm* now = std::localtime(&t);

        fs::path userReportsRoot = fs::path(cfg_.reportsDir)
            .append(activeUserName)
            .append(fmt::format("{}", now->tm_year + 1900));

        // Reports directory structure will look like this
        //
        // ...\kidmon\reports\user\YYYY\snapshots\MM.DD"
        //                             \daily\d-001.txt
        //                             \monthly\m-01.txt
        //                             \weekly\w-01.txt
        //                             \raw\r-001.dat

        ReportDirs dirs;

        dirs.snapshotsDir = (userReportsRoot / "snapshoots").u8string();
        dirs.dailyDir = (userReportsRoot / "daily").u8string();
        dirs.monthlyDir = (userReportsRoot / "monthly").u8string();
        dirs.weeklyDir = (userReportsRoot / "weekly").u8string();
        dirs.rawDir = (userReportsRoot / "raw").u8string();

        auto [it, ok] = dirs_.emplace(activeUserName, std::move(dirs));

        if (!ok)
        {
            static ReportDirs s_dirs;

            return s_dirs;
        }

        return it->second;
    }

public:
    Impl(const Config& cfg)
        : cfg_(cfg)
        , ioc_()
        , timer_(ioc_)
        , workGuard_(ioc_.get_executor())
        , timeoutMs_(3000)
        , api_(ApiFactory::create())
    {
        const auto& dirs = getActiveUserDirs();
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

            spdlog::trace("{}, '{:64}', '{:32}'",
                window->id(),
                window->title(),
                window->className());

            Rect rc = window->boundingRect();

            std::ostringstream oss;
            oss << rc;

            spdlog::trace("Forground wnd: {}", oss.str());
            spdlog::trace("Executable: {}\n", window->ownerProcessPath());

            ImageFormat format = ImageFormat::jpg;
            if (window->capture(format, wndContent_))
            {
                const auto filePath = StringUtils::s2ws(fmt::format("image-{}.{}", ++index_, toString(format)));
                FileUtils::write(filePath, wndContent_);
            }
        }
        catch (const std::exception& e)
        {
            spdlog::error("Failed to cleanup broken enviornment. Desc: {}", e.what());
        }
    }
};

KidMon::KidMon(const Config& cfg)
    : impl_(std::make_unique<Impl>(cfg))
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
