#pragma once

#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <vector>
#include <string>
#include <mutex>

namespace core::utl {

// @todo:hayk - The LogCapture has space for improvement, such as:
// - Keep track of log levels
// - Consider optimizing work with messages
//
// Improvements will be done as I see the need for them in the codebase.

template <typename Mutex>
class MemorySink : public spdlog::sinks::base_sink<Mutex>
{
public:
    MemorySink() = default;
    MemorySink(const MemorySink&) = delete;
    MemorySink& operator=(const MemorySink&) = delete;

    /**
     * @brief Get all captured messages.
     */
    [[nodiscard]] std::vector<std::string> messages() const
    {
        auto lock = std::lock_guard(mtx_);
        return messages_;
    }

    /**
     * @brief Clear captured messages
     */
    void clear()
    {
        auto lock = std::lock_guard(mtx_);
        messages_.clear();
    }

    /**
     * @brief Return the last captured message (if any)
     */
    [[nodiscard]] std::optional<std::string_view> lastMessage() const
    {
        auto lock = std::lock_guard(mtx_);
        if (messages_.empty())
        {
            return std::nullopt;
        }

        return messages_.back();
    }

    [[nodiscard]] bool contains(std::string_view message) const
    {
        auto lock = std::lock_guard(mtx_);
        return std::ranges::any_of(messages_, [message](const auto& msg) {
            return msg.contains(message);
        });
    }

    /**
     * @brief Return the number of captured messages
     */
    [[nodiscard]] size_t count() const
    {
        auto lock = std::lock_guard(mtx_);
        return messages_.size();
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        auto lock = std::lock_guard(mtx_);
        messages_.emplace_back(msg.payload.data(), msg.payload.size());
    }

    void flush_() override {}

private:
    mutable Mutex mtx_;
    std::vector<std::string> messages_;
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
