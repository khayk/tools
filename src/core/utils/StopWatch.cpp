#include <core/utils/StopWatch.h>

StopWatch::StopWatch(const bool autoStart)
{
    if (autoStart)
    {
        start();
    }
}

void StopWatch::start() noexcept
{
    start_ = ClockType::now();
    end_ = start_;
    stopped_ = false;
}

void StopWatch::stop() noexcept
{
    end_ = ClockType::now();
    stopped_ = true;
}

void StopWatch::reset() noexcept
{
    start_ = end_ = ClockType::now();
    stopped_ = true;
}

bool StopWatch::stopped() const noexcept
{
    return stopped_;
}

std::chrono::milliseconds StopWatch::elapsed() const noexcept
{
    auto t = end_;

    if (!stopped_)
    {
        t = ClockType::now();
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(t - start_);
}

int64_t StopWatch::elapsedMs() const noexcept
{
    return elapsed().count();
}
