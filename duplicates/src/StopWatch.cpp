#include "StopWatch.h"

StopWatch::StopWatch(bool autostart)
{
    if (autostart)
    {
        start();
    }
}

void StopWatch::start() noexcept
{
    start_ = Clock::now();
    end_ = start_;
    stopped_ = false;
}

void StopWatch::stop() noexcept
{
    end_ = Clock::now();
    stopped_ = true;
}

bool StopWatch::stopped() const noexcept
{
    return stopped_;
}

uint64_t StopWatch::elapsedMls() const
{
    auto t = end_;

    if (!stopped_)
    {
        t = Clock::now();
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(t - start_).count();
}
