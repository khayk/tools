#pragma once

#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <string>
#include <mutex>

namespace tools::utl {

template <typename Mutex>
class LogSink : public spdlog::sinks::base_sink<Mutex> {
public:
    LogSink() = default;
    LogSink(const LogSink&) = delete;
    LogSink& operator=(const LogSink&) = delete;

    /**
     * @brief Get all captured messages.
     */
    [[nodiscard]] std::vector<std::string> messages() const
    {
        auto lock = std::lock_guard(this->mutex_);
        return messages_;
    }

    /**
     * @brief Clear captured messages
     */
    void clear()
    {
        auto lock = std::lock_guard(this->mutex_);
        messages_.clear();
    }

    /**
     * @brief Return the last captured message (if any)
     */
    [[nodiscard]] std::optional<std::string> lastMessage() const
    {
        auto lock = std::lock_guard(this->mutex_);
        if (messages_.empty())
        {
            return std::nullopt;
        }

        return messages_.back();
    }

    [[nodiscard]] bool contains(std::string_view message) const
    {
        auto lock = std::lock_guard(this->mutex_);
        return std::find(messages_.begin(), messages_.end(), message) != messages_.end();
    }

    /**
     * @brief Return the number of captured messages
     */
    [[nodiscard]] size_t count() const
    {
        auto lock = std::lock_guard(this->mutex_);
        return messages_.size();
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        messages_.emplace_back(msg.payload.data(), msg.payload.size());
    }

    void flush_() override {}

private:
    std::vector<std::string> messages_;
};

// Type alias for common usage (thread-safe)
using TestMtLogSink = LogSink<std::mutex>;

// Type alias for single-threaded usage (if performance is critical and no mt contention)
using TestStLogSink = LogSink<spdlog::details::null_mutex>;

template <typename Mutex>
class LogInterceptor
{
public:
    LogInterceptor()
        : name_{"log_interceptor"}
        , sink_{std::make_shared<LogSink<Mutex>>()}
        , defaultLogger_{spdlog::default_logger()}
    {
        auto logger = std::make_shared<spdlog::logger>(name_, sink_);
        logger->set_level(spdlog::level::trace);
        spdlog::set_default_logger(logger);
    }

    ~LogInterceptor()
    {
        spdlog::set_default_logger(defaultLogger_);
        spdlog::drop(name_);
    }

    LogInterceptor(const LogInterceptor&) = delete;
    LogInterceptor(LogInterceptor&&) = delete;
    LogInterceptor& operator=(const LogInterceptor&) = delete;
    LogInterceptor& operator=(LogInterceptor&&) = delete;

    /**
     * @brief Get the log sink used by this interceptor.
     */
    [[nodiscard]] std::shared_ptr<LogSink<Mutex>> sink() const
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
    const std::string name_;
    std::shared_ptr<LogSink<Mutex>> sink_;
    std::shared_ptr<spdlog::logger> defaultLogger_;
};

// Type alias for common usage (thread-safe)
using TestMtLogInterceptor = LogInterceptor<std::mutex>;

// Type alias for single-threaded usage (if performance is critical and no mt contention)
using TestStLogInterceptor = LogInterceptor<spdlog::details::null_mutex>;

} // namespace tools::utl
