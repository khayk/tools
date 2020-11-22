#pragma once

#include <chrono>

/**
 * @brief Stop watch helper class
 */
class StopWatch
{
    using Clock = std::chrono::system_clock;
    Clock::time_point start_;
    Clock::time_point end_;

    bool stopped_ {true};

public:
    StopWatch(bool autostart = false);

    /**
     * @brief Starts timer
     */
    void start() noexcept;

    /**
     * @brief Stops timer
     */
    void stop() noexcept;

    /**
     * @return true, if the timers stopped, otherwise false
     */
    bool stopped() const noexcept;

    /**
     * @brief Returns time in mls timestamped with start/stop functions
     *        of return elapsed time since timer started (if the stop
     *        is not called)
     */
    uint64_t elapsedMls() const;
};
