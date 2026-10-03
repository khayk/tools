#pragma once

#include <string>

/**
 * @brief Produce trace log records of the given message on scope entry/exit.
 */
class ScopedTrace
{
    static constexpr std::string_view ENTER = "--> ";
    static constexpr std::string_view LEAVE = "<-- ";

    [[nodiscard]] bool shouldTrace() const noexcept;

    std::string message_;
    std::string enter_;
    std::string leave_;
    bool isPrefix_;

public:
    explicit ScopedTrace(const std::string& message,
                         std::string enter = std::string(ENTER),
                         std::string leave = std::string(LEAVE),
                         bool isPrefix = true);
    ~ScopedTrace();

    ScopedTrace(const ScopedTrace&) = delete;
    ScopedTrace(ScopedTrace&&) = delete;
    ScopedTrace& operator=(const ScopedTrace&) = delete;
    ScopedTrace& operator=(ScopedTrace&&) = delete;
};