#pragma once

#include <string>

/**
 * @brief Produce trace log records of the given message on scope entry/exit.
 */
class ScopedTrace
{
    static inline const std::string Enter = "--> ";
    static inline const std::string Leave = "<-- ";

    [[nodiscard]] bool shouldTrace() const noexcept;

    std::string message_;
    std::string enter_;
    std::string leave_;
    bool isPrefix_;

    ScopedTrace(const ScopedTrace&) = delete;
    ScopedTrace(ScopedTrace&&) = delete;
    ScopedTrace& operator=(const ScopedTrace&) = delete;
    ScopedTrace& operator=(ScopedTrace&&) = delete;

public:
    explicit ScopedTrace(const std::string& message,
                         std::string enter = Enter,
                         std::string leave = Leave,
                         bool isPrefix = true);
    ~ScopedTrace() noexcept;
};