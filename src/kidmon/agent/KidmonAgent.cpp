#include <kidmon/agent/KidmonAgent.h>
#include <kidmon/os/Api.h>
#include <kidmon/data/Messages.h>
#include <kidmon/data/Helpers.h>
#include <kidmon/common/Utils.h>

#include <core/utils/FmtExt.h>
#include <core/utils/Str.h>
#include <core/utils/Crypto.h>
#include <core/network/TcpClient.h>
#include <core/network/TcpCommunicator.h>

#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include <unordered_map>
#include <filesystem>
#include <sstream>
#include <chrono>
#include <array>

namespace net = boost::asio;
namespace fs = std::filesystem;

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
            fi.sha256 = crypto::fileSha256(file);
            fi.lastWriteTime = lwt;
        }

        return fi.sha256;
    }
};


class Statistics
{
    uint64_t numEvents_ {0};

public:
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
};


class AgentMsgHandler
{
public:
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

            if (!authReported_)
            {
                nlohmann::json answer;
                jsu::get(js, "answer", answer);
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

    Config cfg_;

    CachedFileSha256 shaCache_;
    net::io_context ioc_;
    net::steady_timer timer_;
    work_guard workGuard_;
    std::chrono::milliseconds timeout_;
    time_point nextCaptureTime_;
    tcp::Client tcpClient_;

    ApiPtr api_;
    std::vector<char> wndContent_;
    std::unique_ptr<tcp::Communicator> comm_;
    AgentMsgHandler handler_;
    Statistics stats_;

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
        tcpClient_.onConnect([this](tcp::Connection& conn) {
            comm_ = std::make_unique<tcp::Communicator>(conn);

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
            msgs::buildAuthMsg(cfg_.authToken, js);
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

        tcp::Client::Options opts {.host = "127.0.0.1",
                                   .port = cfg_.serverPort};
        spdlog::info("Connection attempt to: {}:{}", opts.host, opts.port);

        tcpClient_.connect(opts);
    }

    void collectData()
    {
        timer_.expires_after(timeout_);
        timer_.async_wait([this](const ErrorCode&) {
            collectData();
        });

        try
        {
            stats_.incrementEvents();

            Entry entry;

            auto window = api_->foregroundWindow();
            if (!window)
            {
                spdlog::warn("Unable to detect foreground window");
                return;
            }

            spdlog::trace("collectData");
            spdlog::debug("id: {}, title: {}, class: {}",
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
                spdlog::warn("Unable to retrieve the full path of the processId: {}",
                             window->ownerProcessId());
                return;
            }

            if (cfg_.calcSha)
            {
                entry.processInfo.sha256 =
                    shaCache_.sha256(entry.processInfo.processPath);
            }

            entry.timestamp.capture = SystemClock::now();
            entry.timestamp.duration = timeout_;

            spdlog::debug("Foreground wnd: {}", oss.str());
            spdlog::debug("Executable: {}", entry.processInfo.processPath);

            const auto now = clock_type::now();
            if (cfg_.takeSnapshots && nextCaptureTime_ < now)
            {
                nextCaptureTime_ = now + cfg_.snapshotInterval;
                const ImageFormat format = ImageFormat::jpg;

                if (window->capture(format, wndContent_))
                {
                    std::time_t t = std::time(nullptr);
                    const std::tm tm = utl::timet2tm(t);
                    std::array<char, 16> mbstr{};
                    auto bytesWritten =
                        std::strftime(mbstr.data(), mbstr.size(), "%m%d-%H%M%S", &tm);
                    auto fileName = fmt::format("img-{}.{}",
                                                std::string_view(mbstr.data(), bytesWritten),
                                                toString(format));

                    entry.windowInfo.image.name = fileName;
                    auto data = crypto::encodeBase64(
                        std::string_view(wndContent_.data(), wndContent_.size()));
                    entry.windowInfo.image.bytes = data;
                    entry.windowInfo.image.encoded = true;
                }
            }

            nlohmann::ordered_json js;
            msgs::buildDataMsg(entry, js);
            const auto dataMsg = js.dump();
            spdlog::debug("Sending data message: {}", dataMsg);
            comm_->sendAsync(dataMsg);
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
        std::ostringstream oss;
        stats_.report(oss);
        spdlog::info(oss.str());
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
