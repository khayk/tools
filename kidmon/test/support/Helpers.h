#pragma once

#include <kidmon/data/Messages.h>
#include <kidmon/data/Types.h>

#include <core/app/Runnable.h>
#include <core/network/TcpConnection.h> // IoContext
#include <core/utils/Str.h>
#include <core/utils/Sys.h>

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

// Shared helpers for the agent/server IPC tests: message (de)serialization,
// the active-user lookup the auth handshake depends on, and a small harness for
// running a core::Runnable (KidmonAgent/KidmonServer) on its own thread.
namespace km::test {

using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

inline std::string activeUser()
{
    return core::str::ws2s(core::sys::activeUserName());
}

inline std::string makeAuthMsg(std::string_view token, std::string_view username)
{
    nlohmann::ordered_json js;
    msgs::buildAuthMsg(token, username, js);
    return js.dump();
}

inline std::string makeDataMsg(const Entry& entry)
{
    nlohmann::ordered_json js;
    msgs::buildDataMsg(entry, js);
    return js.dump();
}

// A minimal but valid activity entry for the given user (no snapshot bytes).
inline Entry sampleEntry(std::string_view username)
{
    Entry entry;
    entry.username = std::string(username);
    entry.processInfo.processPath = "proc";
    entry.windowInfo.title = "title";
    entry.timestamp.capture = SystemClock::now();
    entry.timestamp.duration = 1s;
    return entry;
}

// answer.authorized from a server response, if present.
inline std::optional<bool> parseAuthorized(const std::string& msg)
{
    try
    {
        const auto js = nlohmann::json::parse(msg);
        if (js.contains("answer") && js["answer"].contains("authorized"))
        {
            return js["answer"]["authorized"].get<bool>();
        }
    }
    catch (const std::exception&)
    {
    }
    return std::nullopt;
}

// Runs a core::Runnable on its own thread (run() blocks until shutdown, as in
// production) and tracks whether it has returned. Stops and joins on destruction.
class RunnableThread
{
public:
    explicit RunnableThread(core::Runnable& runnable)
        : runnable_(runnable)
        , thread_([this] {
            runnable_.run();
            done_ = true;
        })
    {
    }

    ~RunnableThread()
    {
        stopAndJoin();
    }

    RunnableThread(const RunnableThread&) = delete;
    RunnableThread& operator=(const RunnableThread&) = delete;

    // True once run() has returned (the runnable stopped, on its own or via us).
    [[nodiscard]] bool done() const noexcept
    {
        return done_.load();
    }

    void stopAndJoin()
    {
        runnable_.shutdown(); // no-op if already stopped
        if (thread_.joinable())
        {
            thread_.join();
        }
    }

private:
    core::Runnable& runnable_;
    std::atomic_bool done_ {false};
    std::thread thread_;
};

// Drives ioc on the calling thread until pred() is true or the deadline elapses.
inline void pumpUntil(IoContext& ioc,
                      const std::function<bool()>& pred,
                      Clock::duration max)
{
    const auto deadline = Clock::now() + max;
    while (!pred() && Clock::now() < deadline)
    {
        ioc.run_for(50ms);
    }
}

} // namespace km::test
