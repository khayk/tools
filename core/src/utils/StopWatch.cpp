#include <core/utils/StopWatch.h>

StopWatch::StopWatch(const bool autoStart)
{
    reset();

    if (autoStart)
    {
        start();
    }
}

void StopWatch::start() noexcept
{
    if (!stopped_)
    {
        return;
    }

    start_ = ClockType::now();
    end_ = start_;
    stopped_ = false;
}

void StopWatch::stop() noexcept
{
    if (stopped_)
    {
        return;
    }

    end_ = ClockType::now();
    stopped_ = true;
}

void StopWatch::restart() noexcept
{
    reset();
    start();
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
