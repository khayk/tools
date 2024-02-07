#include <kidmon/agent/KidmonAgent.h>
#include <kidmon/os/Api.h>
#include <kidmon/common/Utils.h>

#include <core/utils/FmtExt.h>
#include <core/utils/Str.h>
#include <core/utils/File.h>
#include <core/utils/Crypto.h>
#include <core/network/TcpClient.h>
#include <core/network/TcpCommunicator.h>

#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <sstream>
#include <chrono>

namespace net = boost::asio;
namespace fs = std::filesystem;

using SystemClock = std::chrono::system_clock;
using TimePoint = SystemClock::time_point;

namespace {

class AgentMsgHandler
{
public:
    using AuthCb = std::function<void(bool)>;
    using MsgCb = std::function<void(const nlohmann::json&)>;
    using ErrCb = std::function<void(int, const std::string&)>;

    bool handle(const std::string& msg)
    {
        int status = 0;
        std::string error;

        try
        {
            nlohmann::json js = nlohmann::json::parse(msg);
            status = js["status"].get<int>();
            error = js["error"].get<std::string>();
            const auto& answer = js["answer"];

            if (!authReported_)
            {
                authCb_(answer["authorized"].get<bool>());
                authReported_ = true;
            }
            else
            {
                msgCb_(js);
            }

            return true;
        }
        catch (const std::exception& ex)
        {
            errCb_(status, ex.what());
        }

        return false;
    }

    void onAuth(AuthCb authCb)
    {
        authCb_ = std::move(authCb);
    }

    void onMsg(MsgCb msgCb)
    {
        msgCb_ = std::move(msgCb);
    }

    void onError(ErrCb errCb)
    {
        errCb_ = std::move(errCb);
    }

private:
    bool authReported_ {false};
    AuthCb authCb_;
    MsgCb msgCb_;
    ErrCb errCb_;
};

} // namespace

struct ProcessInfo
{
    fs::path processPath;
    std::string sha256;
};

struct WindowInfo
{
    std::string title;
    Rect placement;
};

struct Entry
{
    ProcessInfo processInfo;
    WindowInfo windowInfo;
    TimePoint timestamp;
};

void toJson(const ProcessInfo& pi, nlohmann::ordered_json& js)
{
    js["process_path"] = file::path2s(pi.processPath);
    js["sha256"] = pi.sha256;
}

void toJson(const WindowInfo& wi, nlohmann::ordered_json& js)
{
    js["title"] = wi.title;
    js["rect"] = {
        {"leftTop", {wi.placement.leftTop().x(), wi.placement.leftTop().y()}},
        {"dimensions", {wi.placement.width(), wi.placement.height()}}
    };
}

void toJson(const TimePoint& tp, nlohmann::ordered_json& js)
{
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;

    js["since_epoch"] = duration_cast<milliseconds>(tp.time_since_epoch()).count();
    js["unit"] = "mls";
}

void toJson(const Entry& entry, nlohmann::ordered_json& js)
{
    toJson(entry.processInfo, js["process_info"]);
    toJson(entry.windowInfo, js["window_info"]);
    toJson(entry.timestamp, js["timestamp"]);
}

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

void buildAuthMsg(std::string_view authToken, nlohmann::ordered_json& js)
{
    js = {
        {"name", "auth"},
        {"message", {{"username", str::ws2s(sys::activeUserName())},
                     {"token", authToken}}}
    };
}

void buildDataMsg(const Entry& entry, nlohmann::ordered_json& js)
{
    js["name"] = "auth";
    auto& msgJs = js["message"];
    msgJs["username"] = str::ws2s(sys::activeUserName());
    toJson(entry, msgJs["entry"]);
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
    tcp::Client tcpClient_;

    ApiPtr api_;
    std::vector<char> wndContent_;
    std::unordered_map<std::wstring, ReportDirs> dirs_;
    std::unique_ptr<tcp::Communicator> comm_;
    AgentMsgHandler handler_;

    const ReportDirs& getActiveUserDirs()
    {
        std::wstring activeUserName = sys::activeUserName();

        if (auto it = dirs_.find(activeUserName); it != dirs_.end())
        {
            return it->second;
        }

        std::time_t t = std::time(0); // get time now
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

        dirs.snapshotsDir = userReportsRoot / "snapshots";
        dirs.dailyDir = userReportsRoot / "daily";
        dirs.monthlyDir = userReportsRoot / "monthly";
        dirs.weeklyDir = userReportsRoot / "weekly";
        dirs.rawDir = userReportsRoot / "raw";

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

    void initHandlers()
    {
        handler_.onAuth([this](bool succeeds) {
            if (!succeeds)
            {
                spdlog::error("Authorization failed, exiting...");
                ioc_.stop();
            }
            else
            {
                spdlog::info(
                    "Authorization succeeds. Proceeding with data collection...");
                collectData();
            }
        });

        handler_.onMsg([](const nlohmann::json& msg) {
            spdlog::info("Agent processing msg: {}", msg.dump());
            std::ignore = msg;
        });

        handler_.onError([](int status, const std::string& error) {
            spdlog::error("Error in agent handler - status: {}, error: {}",
                          status,
                          error);
        });
    }

    void initiateConnect()
    {
        tcpClient_.onConnect([this](tcp::Connection& conn) {
            comm_ = std::make_unique<tcp::Communicator>(conn);

            comm_->onMsg([this](const std::string& msg) {
                spdlog::info("Agent rcvd: {}", msg);
                handler_.handle(msg);
            });

            conn.onDisconnect([this]() {
                spdlog::info("Agent disconnected.");
                ioc_.stop();
            });

            conn.onError([this](const ErrorCode& ec) {
                spdlog::error("Agent connection error - code: {}, msg: {}",
                              ec.value(),
                              ec.message());
                ioc_.stop();
            });

            comm_->start();

            // Initiate authorization
            nlohmann::ordered_json js;
            buildAuthMsg(cfg_.authToken, js);
            const auto authMsg = js.dump();
            spdlog::trace("Sending auth message: {}", authMsg);
            comm_->sendAsync(authMsg);
        });

        tcpClient_.onError([this](const ErrorCode& ec) {
            spdlog::error("Failed to connect - code: {}, msg: {}",
                          ec.value(),
                          ec.message());
            ioc_.stop();
        });

        tcp::Client::Options opts {"127.0.0.1", cfg_.serverPort};
        tcpClient_.connect(opts);
    }

    void collectData()
    {
        timer_.expires_after(timeout_);
        timer_.async_wait([this](const ErrorCode&) { collectData(); });

        try
        {
            spdlog::trace("Calls collectData");

            Entry entry;

            auto window = api_->foregroundWindow();
            if (!window)
            {
                spdlog::warn("Unable to detect foreground window");
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
                spdlog::warn("Unable to retrieve the path of the process {}",
                             window->ownerProcessId());
                return;
            }

            entry.processInfo.sha256 = crypto::fileSha256(entry.processInfo.processPath);
            entry.timestamp = SystemClock::now();

            spdlog::trace("Foreground wnd: {}", oss.str());
            spdlog::trace("Executable: {}", entry.processInfo.processPath);

            const auto now = clock_type::now();
            if (cfg_.takeSnapshots && nextCaptureTime_ < now)
            {
                nextCaptureTime_ = now + cfg_.snapshotInterval;
                const ImageFormat format = ImageFormat::jpg;

                if (window->capture(format, wndContent_))
                {
                    const auto& userDirs = getActiveUserDirs();
                    std::time_t t = std::time(nullptr);

                    char mbstr[16];
                    auto bytesWritten = std::strftime(mbstr,
                                                      sizeof(mbstr),
                                                      "%m%d-%H%M%S",
                                                      std::localtime(&t));
                    auto fileName = fmt::format("img-{}.{}",
                                                std::string_view(mbstr, bytesWritten),
                                                toString(format));
                    auto file = userDirs.snapshotsDir / fileName;

                    //entry.windowInfo.snapshotPath = file;

                    file::write(file, wndContent_.data(), wndContent_.size());
                }
            }

            nlohmann::ordered_json js;
            buildDataMsg(entry, js);
            const auto dataMsg = js.dump();
            spdlog::trace("Sending data message: {}", dataMsg);
            comm_->sendAsync(dataMsg);
        }
        catch (const std::exception& e)
        {
            spdlog::error("Failure in collectData. Desc: {}", e.what());
        }
    }


public:
    explicit Impl(const Config& cfg)
        : cfg_(cfg)
        , timer_(ioc_)
        , workGuard_(ioc_.get_executor())
        , timeout_(cfg.activityCheckInterval)
        , tcpClient_(ioc_)
        , api_(ApiFactory::create())
    {
    }

    void run()
    {
        using namespace std::chrono_literals;

        // Set to never expire
        timer_.expires_at(time_point::max());

        initHandlers();
        initiateConnect();

        ioc_.run();
    }

    void shutdown() noexcept
    {
        ioc_.stop();
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
    spdlog::trace("Running KidmonAgent");

    impl_->run();
}

void KidmonAgent::shutdown() noexcept
{
    spdlog::trace("Shutdown requested");

    impl_->shutdown();
}
