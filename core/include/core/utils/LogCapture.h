#pragma once

#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <optional>
#include <vector>
#include <string>
#include <mutex>

namespace core::utl {

template <typename Mutex>
class MemorySink : public spdlog::sinks::base_sink<Mutex>
{
public:
    struct LogEntry
    {
        spdlog::level::level_enum level {spdlog::level::off};
        std::string message;
    };

    MemorySink() = default;
    MemorySink(const MemorySink&) = delete;
    MemorySink& operator=(const MemorySink&) = delete;

    /**
     * @brief Get all captured log entries.
     */
    [[nodiscard]] std::vector<LogEntry> messages() const
    {
        std::lock_guard lock(mtx_);
        return entries_;
    }

    /**
     * @brief Clear captured messages
     */
    void clear()
    {
        std::lock_guard lock(mtx_);
        entries_.clear();
    }

    /**
     * @brief Return the last captured message (if any)
     */
    [[nodiscard]] std::optional<std::string_view> lastMessage() const
    {
        std::lock_guard lock(mtx_);
        if (entries_.empty())
        {
            return std::nullopt;
        }

        return entries_.back().message;
    }

    [[nodiscard]] bool contains(std::string_view message) const
    {
        std::lock_guard lock(mtx_);
        return std::ranges::any_of(entries_, [message](const auto& entry) {
            return entry.message.contains(message);
        });
    }

    [[nodiscard]] bool contains(std::string_view message,
                                spdlog::level::level_enum level) const
    {
        std::lock_guard lock(mtx_);
        return std::ranges::any_of(entries_, [message, level](const auto& entry) {
            return entry.level == level && entry.message.contains(message);
        });
    }

    /**
     * @brief Return the number of captured messages
     */
    [[nodiscard]] size_t count() const
    {
        std::lock_guard lock(mtx_);
        return entries_.size();
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        std::lock_guard lock(mtx_);
        entries_.emplace_back(msg.level,
                              std::string(msg.payload.data(), msg.payload.size()));
    }

    void flush_() override {}

private:
    mutable Mutex mtx_;
    std::vector<LogEntry> entries_;
};

// Type alias for common usage (thread-safe)
using TestMtLogSink = MemorySink<std::mutex>;

// Type alias for single-threaded usage (if performance is critical and no mt
// contention)
using TestStLogSink = MemorySink<spdlog::details::null_mutex>;

// RAII sink that captures spdlog output — useful in unit tests to verify
// that a function emits the expected log messages.
template <typename Mutex>
class LogCapture
{
public:
    using LogEntry = typename MemorySink<Mutex>::LogEntry;

    LogCapture(spdlog::level::level_enum level = spdlog::level::trace)
    {
        auto sink = std::make_shared<MemorySink<Mutex>>();
        sink->set_pattern("%v");
        sink->set_level(spdlog::level::trace);
        sink_ = sink;

        auto logger = spdlog::default_logger();
        prevSinks_ = logger->sinks();
        logger->sinks() = {sink_};
        spdlog::set_level(level);
    }

    ~LogCapture()
    {
        spdlog::default_logger()->sinks() = prevSinks_;
        spdlog::set_level(prevLevel_);
    }

    LogCapture(const LogCapture&) = delete;
    LogCapture(LogCapture&&) = delete;
    LogCapture& operator=(const LogCapture&) = delete;
    LogCapture& operator=(LogCapture&&) = delete;

    /**
     * @brief Get the log sink used by this interceptor.
     */
    [[nodiscard]] std::shared_ptr<MemorySink<Mutex>> sink() const
    {
        return sink_;
    }

    [[nodiscard]] std::vector<LogEntry> messages() const
    {
        return sink_->messages();
    }

    [[nodiscard]] std::optional<std::string_view> lastMessage() const
    {
        return sink_->lastMessage();
    }

    /**
     * @brief Get the number of captured messages.
     */
    [[nodiscard]] size_t count() const
    {
        return sink_->count();
    }

    /**
     * @brief Returns true if the interceptor contains the given message.
     */
    [[nodiscard]] bool contains(std::string_view message) const
    {
        return sink_->contains(message);
    }

    [[nodiscard]] bool contains(std::string_view message,
                                spdlog::level::level_enum level) const
    {
        return sink_->contains(message, level);
    }

private:
    spdlog::level::level_enum prevLevel_ {spdlog::get_level()};
    std::vector<spdlog::sink_ptr> prevSinks_;
    std::shared_ptr<MemorySink<Mutex>> sink_;
};

// Type alias for common usage (thread-safe)
using LogCaptureMt = LogCapture<std::mutex>;

// Type alias for single-threaded usage (if performance is critical and no mt
// contention)
using LogCaptureSt = LogCapture<spdlog::details::null_mutex>;

} // namespace core::utl
