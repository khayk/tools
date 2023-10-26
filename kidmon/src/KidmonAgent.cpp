#include "KidmonAgent.h"
#include "os/Api.h"
#include "common/Utils.h"
#include "common/FmtExt.h"

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
    fs::path processPath;
    std::string sha256;
};

struct WindowInfo
{
    fs::path snapshotPath;
    std::string title;
    Rect placement;
};

struct Entry
{
    ProcessInfo processInfo;
    WindowInfo windowInfo;
    TimePoint timestamp;
};

struct ReportDirs
{
    fs::path snapshotsDir;
    fs::path dailyDir;
    fs::path monthlyDir;
    fs::path weeklyDir;
    fs::path rawDir;
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

class KidmonAgent::Impl
{
    using work_guard = net::executor_work_guard<net::io_context::executor_type>;
    using clock_type = net::steady_timer::clock_type;
    using time_point = net::steady_timer::time_point;

    const Config& cfg_;

    net::io_context ioc_;
    net::steady_timer timer_;
    work_guard workGuard_;
    std::chrono::milliseconds timeout_;
    time_point nextCaptureTime_;

    ApiPtr api_;
    std::vector<char> wndContent_;
    size_t index_{ 0 };
    size_t bufferedBytes_ {0};

    std::vector<Entry> cachedEntries_;

    std::unordered_map<std::wstring, ReportDirs> dirs_;

    const ReportDirs& getActiveUserDirs()
    {
        std::wstring activeUserName = sys::activeUserName();

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

        dirs.snapshotsDir   = userReportsRoot / "snapshoots";
        dirs.dailyDir       = userReportsRoot / "daily";
        dirs.monthlyDir     = userReportsRoot / "monthly";
        dirs.weeklyDir      = userReportsRoot / "weekly";
        dirs.rawDir         = userReportsRoot / "raw";

        fs::create_directories(dirs.snapshotsDir);
        fs::create_directories(dirs.dailyDir);
        fs::create_directories(dirs.monthlyDir);
        fs::create_directories(dirs.weeklyDir);
        fs::create_directories(dirs.rawDir);

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
        , timeout_(cfg.activityCheckInterval)
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
        using namespace str;

        timer_.expires_after(timeout_);
        timer_.async_wait(std::bind(&Impl::collectData, this));

        try
        {
            spdlog::trace("Calls collectData");

            Entry entry;

            auto window = api_->forgroundWindow();
            if (!window)
            {
                spdlog::warn("Unable to detect forground window");
                return;
            }

            spdlog::trace("{}, '{}', '{}'",
                window->id(),
                window->title(),
                window->className());

            Rect rc = window->boundingRect();

            std::ostringstream oss;
            oss << rc;

            entry.windowInfo.placement = window->boundingRect();
            entry.windowInfo.title = window->title();
            entry.processInfo.processPath = window->ownerProcessPath();

            if (entry.processInfo.processPath.empty())
            {
                spdlog::warn("Unable to retrieve the path of the process {}", window->ownerProcessId());
                return;
            }

            entry.processInfo.sha256 = crypto::fileSha256(entry.processInfo.processPath);

            spdlog::trace("Forground wnd: {}", oss.str());
            spdlog::trace("Executable: {}\n", entry.processInfo.processPath);

            if (nextCaptureTime_ < clock_type::now())
            {
                nextCaptureTime_ = clock_type::now() + cfg_.snapshotInterval;
                ImageFormat format = ImageFormat::jpg;

                if (window->capture(format, wndContent_))
                {
                    const auto& userDirs = getActiveUserDirs();
                    std::time_t t = std::time(nullptr);

                    char mbstr[16];
                    auto bytesWritten = std::strftime(mbstr, sizeof(mbstr), "%m%d-%H%M%S", std::localtime(&t));
                    auto fileName = fmt::format("img-{}.{}", std::string_view(mbstr, bytesWritten), toString(format));
                    auto file = userDirs.snapshotsDir / fileName;

                    entry.windowInfo.snapshotPath = file;
                    file::write(file, wndContent_);
                }
            }

            cachedEntries_.push_back(std::move(entry));
        }
        catch (const std::exception& e)
        {
            spdlog::error("Failed to cleanup broken enviornment. Desc: {}", e.what());
        }
    }
};

KidmonAgent::KidmonAgent(const Config& cfg)
    : impl_(std::make_unique<Impl>(cfg))
{
}

KidmonAgent::~KidmonAgent()
{
    impl_.reset();
}

void KidmonAgent::run()
{
    spdlog::trace("Running KidmonAgent application");

    impl_->run();
}

void KidmonAgent::shutdown() noexcept
{
    spdlog::trace("Shutdown requested");

    impl_->shutdown();
}
