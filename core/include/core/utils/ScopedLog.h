#pragma once

#include <spdlog/common.h>

#include <string>

/**
 * @brief Produce log records of the given message and level on scope entry/exit.
 */
class ScopedLog
{
    static constexpr std::string_view ENTER = "--> ";
    static constexpr std::string_view LEAVE = "<-- ";

    [[nodiscard]] bool shouldLog() const noexcept;

    std::string message_;
    std::string enter_;
    std::string leave_;
    spdlog::level::level_enum level_;
    bool isPrefix_;

public:
    explicit ScopedLog(const std::string& message,
                       spdlog::level::level_enum level = spdlog::level::trace,
                       std::string enter = std::string(ENTER),
                       std::string leave = std::string(LEAVE),
                       bool isPrefix = true);
    ~ScopedLog();

    ScopedLog(const ScopedLog&) = delete;
    ScopedLog(ScopedLog&&) = delete;
    ScopedLog& operator=(const ScopedLog&) = delete;
    ScopedLog& operator=(ScopedLog&&) = delete;
};
