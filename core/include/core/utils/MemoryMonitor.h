#pragma once

#include <spdlog/spdlog.h>
#include <cstddef>
#include <cstdint>
#include <string>

namespace core::mem {

/**
 * @brief A point-in-time snapshot of the current process' memory footprint
 */
struct MemoryStats
{
    size_t currentBytes {}; //!< Resident memory currently occupied by the process
    size_t peakBytes {};    //!< Highest resident memory reached since process start
};

/**
 * @brief Takes a snapshot of the current process' memory usage
 */
MemoryStats currentMemoryStats();

/**
 * @brief Formats a byte count as a human readable string, e.g. "132.4 MB"
 *
 * @param bytes      Number of bytes to format
 * @param precision  Number of digits after the decimal point
 */
std::string humanReadableBytes(int64_t bytes, int precision = 1);

/**
 * @brief Tracks memory usage of the current process relative to a baseline,
 *        similar in spirit to StopWatch but for memory instead of time.
 *
 * The baseline is captured on construction (unless autoStart is false) or on a
 * subsequent call to start()/restart(). current() and deltaBytes() can then be
 * queried at any time without affecting the baseline.
 */
class MemoryMonitor
{
    MemoryStats baseline_;
    bool started_ {false};

public:
    /**
     * @brief Captures a baseline automatically if autoStart is specified
     */
    explicit MemoryMonitor(bool autoStart = true);

    /**
     * @brief Captures a new baseline. No-op if already started
     */
    void start();

    /**
     * @brief Captures a new baseline unconditionally
     */
    void restart();

    /**
     * @brief Clears the baseline, monitor becomes not started
     */
    void reset() noexcept;

    /**
     * @return true, if a baseline has been captured, otherwise false
     */
    [[nodiscard]] bool started() const noexcept;

    /**
     * @return Memory stats captured as the baseline
     */
    [[nodiscard]] MemoryStats baseline() const noexcept;

    /**
     * @return A fresh memory snapshot taken right now
     */
    [[nodiscard]] static MemoryStats current();

    /**
     * @brief Difference between the current resident memory and the baseline.
     *        Positive means the process grew since the baseline was captured,
     *        negative means it shrank.
     */
    [[nodiscard]] int64_t deltaBytes() const;

    /**
     * @return Highest resident memory observed since the baseline was captured
     */
    [[nodiscard]] static size_t peakBytes();
};

/**
 * @brief Logs the memory footprint of the current process on scope entry/exit,
 *        along with the delta and peak observed across the scope.
 */
class ScopedMemoryTrace
{
    std::string message_;
    MemoryMonitor monitor_;
    spdlog::level::level_enum level_;

    [[nodiscard]] static bool shouldTrace() noexcept;

public:
    explicit ScopedMemoryTrace(std::string message,
                               spdlog::level::level_enum level = spdlog::level::debug);
    ~ScopedMemoryTrace();

    ScopedMemoryTrace(const ScopedMemoryTrace&) = delete;
    ScopedMemoryTrace(ScopedMemoryTrace&&) = delete;
    ScopedMemoryTrace& operator=(const ScopedMemoryTrace&) = delete;
    ScopedMemoryTrace& operator=(ScopedMemoryTrace&&) = delete;
};

} // namespace core::mem
