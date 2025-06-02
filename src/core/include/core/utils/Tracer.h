#pragma once

#include <string>

/**
 * @brief Produce trace log records of the given message on scope entry/exit.
 */
class ScopedTrace
{
    inline static const std::string ENTER = "--> ";
    inline static const std::string LEAVE = "<-- ";

    [[nodiscard]] bool shouldTrace() const noexcept;

    std::string message_;
    std::string enter_;
    std::string leave_;
    bool isPrefix_;

public:
    explicit ScopedTrace(const std::string& message,
                         std::string enter = ENTER,
                         std::string leave = LEAVE,
                         bool isPrefix = true);
    ~ScopedTrace();

    ScopedTrace(const ScopedTrace&) = delete;
    ScopedTrace(ScopedTrace&&) = delete;
    ScopedTrace& operator=(const ScopedTrace&) = delete;
    ScopedTrace& operator=(ScopedTrace&&) = delete;
};