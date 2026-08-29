#include <core/utils/MemoryMonitor.h>
#include <core/utils/Sys.h>
#include <array>
#include <format>
#include <iostream>

namespace core::mem {

MemoryStats currentMemoryStats()
{
    return {.currentBytes = core::sys::currentProcessMemoryUsage(),
            .peakBytes = core::sys::currentProcessPeakMemoryUsage()};
}

std::string humanReadableBytes(const int64_t bytes, const int precision)
{
    static constexpr std::array UNITS {"B", "KB", "MB", "GB", "TB", "PB"};

    const bool negative = bytes < 0;
    auto value = static_cast<double>(negative ? -bytes : bytes);

    size_t unit = 0;
    while (value >= 1024.0 && unit + 1 < UNITS.size())
    {
        value /= 1024.0;
        ++unit;
    }

    return std::format("{}{:.{}f} {}",
                       negative ? "-" : "",
                       value,
                       precision,
                       UNITS.at(unit));
}

MemoryMonitor::MemoryMonitor(const bool autoStart)
{
    if (autoStart)
    {
        start();
    }
}

void MemoryMonitor::start()
{
    if (started_)
    {
        return;
    }

    restart();
}

void MemoryMonitor::restart()
{
    baseline_ = currentMemoryStats();
    started_ = true;
}

void MemoryMonitor::reset() noexcept
{
    baseline_ = {};
    started_ = false;
}

bool MemoryMonitor::started() const noexcept
{
    return started_;
}

MemoryStats MemoryMonitor::baseline() const noexcept
{
    return baseline_;
}

MemoryStats MemoryMonitor::current()
{
    return currentMemoryStats();
}

int64_t MemoryMonitor::deltaBytes() const
{
    return static_cast<int64_t>(current().currentBytes) -
           static_cast<int64_t>(baseline_.currentBytes);
}

size_t MemoryMonitor::peakBytes()
{
    return current().peakBytes;
}

bool ScopedMemoryTrace::shouldTrace() noexcept
{
    return spdlog::default_logger_raw() != nullptr;
}

ScopedMemoryTrace::ScopedMemoryTrace(std::string message,
                                     const spdlog::level::level_enum level)
    : message_(std::move(message))
    , monitor_(true)
    , level_(level)
{
    if (shouldTrace())
    {
        spdlog::log(level_,
                    "--> {} memory: {}",
                    message_,
                    humanReadableBytes(
                        static_cast<int64_t>(monitor_.baseline().currentBytes)));
    }
}

ScopedMemoryTrace::~ScopedMemoryTrace()
{
    try
    {
        if (shouldTrace())
        {
            spdlog::log(
                level_,
                "<-- {} memory: {} ({}{}, peak {})",
                message_,
                humanReadableBytes(
                    static_cast<int64_t>(MemoryMonitor::current().currentBytes)),
                monitor_.deltaBytes() >= 0 ? "+" : "",
                humanReadableBytes(monitor_.deltaBytes()),
                humanReadableBytes(static_cast<int64_t>(MemoryMonitor::peakBytes())));
        }
    }
    catch (...)
    {
        std::cerr << "Failed to log memory trace message" << '\n';
    }
}

} // namespace core::mem
