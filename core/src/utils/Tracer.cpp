#include <core/utils/Tracer.h>

#include <spdlog/spdlog.h>

std::string extractFunction(const std::string& fullyQualifiedName)
{
    const auto p = fullyQualifiedName.find_first_of("::");

    if (p != std::string::npos)
    {
        return fullyQualifiedName.substr(p + 2);
    }

    return fullyQualifiedName;
}

bool ScopedTrace::shouldTrace() const noexcept
{
    return spdlog::default_logger_raw() &&
           (!message_.empty() || !enter_.empty() || !leave_.empty());
}

ScopedTrace::ScopedTrace(const std::string& message,
                         std::string enter,
                         std::string leave,
                         const bool isPrefix)
    : message_(extractFunction(message))
    , enter_(std::move(enter))
    , leave_(std::move(leave))
    , isPrefix_(isPrefix)
{
    if (shouldTrace())
    {
        if (isPrefix_)
        {
            spdlog::trace("{}{}", enter_, message_);
        }
        else
        {
            spdlog::trace("{}{}", message_, enter_);
        }
    }
}

ScopedTrace::~ScopedTrace() noexcept
{
    try
    {
        if (shouldTrace())
        {
            if (isPrefix_)
            {
                spdlog::trace("{}{}", leave_, message_);
            }
            else
            {
                spdlog::trace("{}{}", message_, leave_);
            }
        }
    }
    catch (...)
    {
    }
}