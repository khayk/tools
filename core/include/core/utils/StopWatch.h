#pragma once

#include <chrono>

/**
 * @brief Stop watch helper class
 */
class StopWatch
{
    using ClockType = std::chrono::steady_clock;

    ClockType::time_point start_;
    ClockType::time_point end_;
    bool stopped_ {true};

public:
    /**
     * @brief Start a watch automatically if autoStart is specified
     */
    explicit StopWatch(bool autoStart = true);

    /**
     * @brief Starts timer
     */
    void start() noexcept;

    /**
     * @brief Stops timer
     */
    void stop() noexcept;

    /**
     * @brief Resets timer
     */
    void reset() noexcept;

    /**
     * @return true, if the timers stopped, otherwise false
     */
    bool stopped() const noexcept;

    /**
     * @return Elapsed time since timer is started
     */
    std::chrono::milliseconds elapsed() const noexcept;

    /**
     * @brief Returns time in milliseconds timestamped with start/stop functions
     *        of return elapsed time since timer started (if the stop
     *        is not called)
     */
    int64_t elapsedMs() const noexcept;
};
