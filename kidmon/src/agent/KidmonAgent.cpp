#include <kidmon/agent/KidmonAgent.h>
#include <kidmon/os/Api.h>
#include <kidmon/data/Messages.h>
#include <kidmon/data/Helpers.h>
#include <kidmon/common/Utils.h>
#include <kidmon/common/Defaults.h>

#include <core/utils/FmtExt.h>
#include <core/utils/Str.h>
#include <core/utils/File.h>
#include <core/utils/Crypto.h>
#include <core/network/TcpClient.h>
#include <core/network/TcpCommunicator.h>
#include <core/utils/Sys.h>
#include <core/utils/StopWatch.h>

#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include <unordered_map>
#include <filesystem>
#include <sstream>
#include <chrono>
#include <array>
#include <algorithm>
#include <format>
#include <optional>
#include <vector>

namespace net = boost::asio;
namespace fs = std::filesystem;

namespace km {

namespace {

class CachedFileSha256
{
    struct FileInfo
    {
        std::string sha256;
        std::filesystem::file_time_type lastWriteTime;
    };

    mutable std::unordered_map<fs::path, FileInfo> cachedSha_;

public:
    const std::string& sha256(const fs::path& file) const
    {
        const auto lwt = std::filesystem::last_write_time(file);
        auto [it, _] = cachedSha_.emplace(file, FileInfo {});
        auto& fi = it->second;

        // Update SHA256 only if the file is changed or newly added
        if (fi.lastWriteTime != lwt)
        {
            fi.sha256 = core::crypto::fileSha256(file);
            fi.lastWriteTime = lwt;
        }

        return fi.sha256;
    }
};


class Statistics
{
    using Clock = std::chrono::steady_clock;

    static constexpr std::size_t TOP_APPS = 3;

    uint64_t numEvents_ {0};

    // Counters for the periodic status line, reset by maybeReport().
    std::chrono::milliseconds reportInterval_;
    Clock::time_point lastReport_ {Clock::now()};
    uint64_t entries_ {0};
    uint64_t heartbeats_ {0};
    uint64_t skipped_ {0};
    uint64_t snapshots_ {0};
    std::unordered_map<std::string, std::chrono::milliseconds> appTime_;

    std::string topApps()
    {
        using AppTime = std::pair<std::string, std::chrono::milliseconds>;
        std::vector<AppTime> apps(appTime_.begin(), appTime_.end());
        const auto top = std::min(apps.size(), TOP_APPS);
        std::partial_sort(apps.begin(),
                          apps.begin() + static_cast<std::ptrdiff_t>(top),
                          apps.end(),
                          [](const AppTime& a, const AppTime& b) {
                              return a.second > b.second;
                          });

        std::string out;
        for (std::size_t i = 0; i < top; ++i)
        {
            out += std::format("{}{} {}",
                               i == 0 ? "" : ", ",
                               apps[i].first,
                               core::str::humanizeDuration(apps[i].second));
        }

        return out.empty() ? "-" : out;
    }

public:
    explicit Statistics(std::chrono::milliseconds reportInterval)
        : reportInterval_(reportInterval)
    {
    }

    void report(std::ostream& oss) const
    {
        oss << "Total number of events collected: " << numEvents_;
    }

    void incrementEvents()
    {
        ++numEvents_;
    }

    [[nodiscard]] uint64_t numEvents() const noexcept
    {
        return numEvents_;
    }

    void addEntry(const fs::path& processPath, std::chrono::milliseconds duration)
    {
        ++entries_;
        appTime_[core::file::path2s(processPath.filename())] += duration;
    }

    void addHeartbeat()
    {
        ++heartbeats_;
    }

    void addSkipped()
    {
        ++skipped_;
    }

    void addSnapshot()
    {
        ++snapshots_;
    }

    // Logs an info-level activity summary and resets the window counters once
    // the report interval has elapsed.
    void maybeReport(std::chrono::milliseconds uptime)
    {
        using core::str::humanizeDuration;
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;

        const auto now = Clock::now();
        if (now - lastReport_ < reportInterval_)
        {
            return;
        }

        spdlog::info("Last {}: {} entries, {} idle heartbeats, {} skipped, "
                     "{} snapshots | top: {} | uptime {}",
                     humanizeDuration(duration_cast<milliseconds>(now - lastReport_)),
                     entries_,
                     heartbeats_,
                     skipped_,
                     snapshots_,
                     topApps(),
                     humanizeDuration(uptime));

        lastReport_ = now;
        entries_ = heartbeats_ = skipped_ = snapshots_ = 0;
        appTime_.clear();
    }
};


class AgentMsgHandler
{
public:
    enum class State : uint8_t
    {
        WaitingAuth,
        Authorized
    };

    using AuthCb = std::function<void(bool)>;
    using MsgCb = std::function<void(const nlohmann::ordered_json&)>;
    using ErrCb = std::function<void(int, const std::string&)>;

    bool handle(const std::string& msg)
    {
        int status = 0;
        std::string error;

        try
        {
            const auto js = nlohmann::ordered_json::parse(msg);
            jsu::get(js, "status", status);
            jsu::get(js, "error", error, status != 0);

            switch (state_)
            {
                case State::WaitingAuth:
                {
                    if (status != 0)
                    {
                        // Server rejected authorization (e.g. bad token, or an
                        // agent is already connected). Surface the reason, then fail
                        // authorization so shutdown is driven deterministically here
                        // rather than depending on the server closing the socket.
                        spdlog::error("Authorization rejected - status: {}, error: {}",
                                      status,
                                      error);
                        authCb_(false);
                        break;
                    }
                    nlohmann::json answer;
                    jsu::get(js, "answer", answer, false);
                    authCb_(answer.value("authorized", false));
                    state_ = State::Authorized;
                    break;
                }
                case State::Authorized:
                    if (status != 0)
                    {
                        errCb_(status, error);
                    }
                    else
                    {
                        msgCb_(js);
                    }
                    break;
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
    State state_ {State::WaitingAuth};
    AuthCb authCb_;
    MsgCb msgCb_;
    ErrCb errCb_;
};

} // namespace

class KidmonAgent::Impl
{
    using work_guard = net::executor_work_guard<net::io_context::executor_type>;
    using clock_type = net::steady_timer::clock_type;
    using time_point = net::steady_timer::time_point;

    Config cfg_;

    // The active user does not change for the lifetime of the agent process, so
    // resolve it once and reuse it for the auth handshake and every data message.
    const std::string username_ = core::str::ws2s(core::sys::activeUserName());

    CachedFileSha256 shaCache_;
    net::io_context ioc_;
    net::steady_timer timer_;
    work_guard workGuard_;
    std::chrono::milliseconds timeout_;
    time_point nextCaptureTime_;
    const StopWatch upTime_;
    core::tcp::Client tcpClient_;

    ApiPtr api_;
    std::vector<char> wndContent_;
    std::unique_ptr<core::tcp::Communicator> comm_;
    AgentMsgHandler handler_;
    Statistics stats_ {cfg_.statusReportInterval};

    // Set while the user is away; when the away period started.
    std::optional<time_point> awaySince_;

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
                    "Authorization succeeded, proceeding with data collection...");
                collectData();
            }
        });

        handler_.onMsg([](const nlohmann::ordered_json& msg) {
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
        tcpClient_.onConnect([this](core::tcp::Connection& conn) {
            comm_ = std::make_unique<core::tcp::Communicator>(conn);

            comm_->onMsg([this](const std::string& msg) {
                spdlog::debug("Agent rcvd: {}", msg);
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
            msgs::buildAuthMsg(cfg_.authToken, username_, js);
            const auto authMsg = js.dump();
            spdlog::debug("Sending auth message: {}", authMsg);
            comm_->sendAsync(authMsg);
        });

        tcpClient_.onError([this](const ErrorCode& ec) {
            spdlog::error("Failed to connect - code: {}, msg: {}",
                          ec.value(),
                          ec.message());
            ioc_.stop();
        });

        core::tcp::Client::Options opts {.host = "127.0.0.1", .port = cfg_.serverPort};
        spdlog::info("Connection attempt to: {}:{}", opts.host, opts.port);

        tcpClient_.connect(opts);
    }

    // Send a heartbeat in place of an activity entry. Keeps the connection
    // alive while the user is away without inflating time-on-task figures.
    void sendHeartbeat(std::chrono::milliseconds idle)
    {
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;

        const auto nowEpoch =
            duration_cast<milliseconds>(SystemClock::now().time_since_epoch());
        const auto lastActivity = nowEpoch - idle;

        nlohmann::ordered_json js;
        msgs::buildHeartbeat(upTime_.elapsed().count(), lastActivity.count(), js);
        const auto hb = js.dump();
        spdlog::debug("User idle for {} ms; sending heartbeat", idle.count());
        comm_->sendAsync(hb);
        stats_.addHeartbeat();
    }

    void collectData()
    {
        timer_.expires_after(timeout_);
        timer_.async_wait([this](const ErrorCode&) {
            collectData();
        });

        const auto now = clock_type::now();
        stats_.maybeReport(upTime_.elapsed());

        try
        {
            using core::str::humanizeDuration;

            // While the user is away, do not record activity (it would count the
            // same window for hours). Send a heartbeat instead so the server
            // keeps the connection and knows we are still alive.
            //
            // "Away" means no keyboard/mouse input past the threshold AND the
            // display has gone to sleep. Keeping the screen-on case active counts
            // passive presence (reading, watching a movie, a video call) even with
            // no input, until the OS display-sleep timeout fires once truly away.
            const auto idle = api_->idleTime();
            if (idle >= cfg_.idleThreshold && !api_->displayOn())
            {
                if (!awaySince_)
                {
                    awaySince_ = now - idle;
                    spdlog::info("User away (no input for {}, display off)",
                                 humanizeDuration(idle));
                }

                sendHeartbeat(idle);
                return;
            }

            if (awaySince_)
            {
                spdlog::info("User active again after {}",
                             humanizeDuration(
                                 std::chrono::duration_cast<std::chrono::milliseconds>(
                                     now - *awaySince_)));
                awaySince_.reset();
            }

            stats_.incrementEvents();

            Entry entry;

            auto window = api_->foregroundWindow();
            if (!window)
            {
                spdlog::warn("Unable to detect foreground window");
                stats_.addSkipped();
                return;
            }

            spdlog::trace("collectData");
            spdlog::debug("id: {}, title: {}", window->id(), window->title());

            entry.windowInfo.placement = window->boundingRect();
            entry.windowInfo.title = window->title();
            entry.processInfo.processPath = window->ownerProcessPath();

            if (entry.processInfo.processPath.empty())
            {
                spdlog::warn("Unable to retrieve the full path of the processId: {}",
                             window->ownerProcessId());
                stats_.addSkipped();
                return;
            }

            if (cfg_.calcSha)
            {
                entry.processInfo.sha256 =
                    shaCache_.sha256(entry.processInfo.processPath);
            }

            entry.timestamp.capture = SystemClock::now();
            entry.timestamp.duration = timeout_;

            std::ostringstream oss;
            oss << entry.windowInfo.placement;
            spdlog::debug("Foreground wnd: {}", oss.str());
            spdlog::debug("Executable: {}", entry.processInfo.processPath);

            if (cfg_.takeSnapshots && nextCaptureTime_ < now)
            {
                nextCaptureTime_ = now + cfg_.snapshotInterval;
                const ImageFormat format = ImageFormat::jpg;

                if (window->capture(format, wndContent_))
                {
                    std::time_t t = std::time(nullptr);
                    const std::tm tm = utl::timet2tm(t);
                    std::array<char, 16> mbstr {};
                    auto bytesWritten =
                        std::strftime(mbstr.data(), mbstr.size(), "%m%d-%H%M%S", &tm);
                    auto fileName =
                        std::format("img-{}.{}",
                                    std::string_view(mbstr.data(), bytesWritten),
                                    toString(format));

                    entry.windowInfo.image.name = fileName;
                    auto data = core::crypto::encodeBase64(
                        std::string_view(wndContent_.data(), wndContent_.size()));
                    entry.windowInfo.image.bytes = data;
                    entry.windowInfo.image.encoded = true;
                    stats_.addSnapshot();
                }
            }

            entry.username = username_;
            nlohmann::ordered_json js;
            msgs::buildDataMsg(entry, js);
            const auto dataMsg = js.dump();
            spdlog::debug("Sending data message: {}", dataMsg);
            comm_->sendAsync(dataMsg);
            stats_.addEntry(entry.processInfo.processPath, timeout_);
        }
        catch (const std::exception& e)
        {
            spdlog::error("Failure in collectData. Desc: {}", e.what());
        }
    }

public:
    explicit Impl(Config cfg)
        : cfg_(std::move(cfg))
        , timer_(ioc_)
        , workGuard_(ioc_.get_executor())
        , timeout_(cfg_.activityCheckInterval)
        , tcpClient_(ioc_)
        , api_(ApiFactory::create())
    {
    }

    ~Impl()
    {
        try
        {
            std::ostringstream oss;
            stats_.report(oss);
            spdlog::info(oss.str());
        }
        catch (const std::exception& e)
        {
            spdlog::error("Exception in ~KidmonAgent::Impl:", e.what());
        }
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

KidmonAgent::Config::Config()
    : activityCheckInterval {defaults::ACTIVITY_CHECK_INTERVAL}
    , snapshotInterval {defaults::SNAPSHOT_INTERVAL}
    , idleThreshold {defaults::IDLE_THRESHOLD}
    , statusReportInterval {defaults::STATUS_REPORT_INTERVAL}
    , serverPort {defaults::SERVER_PORT}
{
}

KidmonAgent::KidmonAgent(Config cfg)
    : impl_(std::make_unique<Impl>(std::move(cfg)))
{
}

KidmonAgent::~KidmonAgent()
{
    impl_.reset();
}

void KidmonAgent::run()
{
    spdlog::info("Running KidmonAgent");
    impl_->run();
}

void KidmonAgent::shutdown() noexcept
{
    spdlog::info("Shutdown requested");
    impl_->shutdown();
}

} // namespace km
