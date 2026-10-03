#include <core/utils/ScopedLog.h>
#include <spdlog/spdlog.h>
#include <iostream>

std::string extractFunction(const std::string& fullyQualifiedName)
{
    const auto p = fullyQualifiedName.find_first_of("::");

    if (p != std::string::npos)
    {
        return fullyQualifiedName.substr(p + 2);
    }

    return fullyQualifiedName;
}

bool ScopedLog::shouldLog() const noexcept
{
    return spdlog::default_logger_raw() &&
           (!message_.empty() || !enter_.empty() || !leave_.empty());
}

ScopedLog::ScopedLog(const std::string& message,
                     spdlog::level::level_enum level,
                     std::string enter,
                     std::string leave,
                     const bool isPrefix)
    : message_(extractFunction(message))
    , enter_(std::move(enter))
    , leave_(std::move(leave))
    , level_(level)
    , isPrefix_(isPrefix)
{
    if (shouldLog())
    {
        if (isPrefix_)
        {
            spdlog::log(level_, "{}{}", enter_, message_);
        }
        else
        {
            spdlog::log(level_, "{}{}", message_, enter_);
        }
    }
}

ScopedLog::~ScopedLog()
{
    try
    {
        if (shouldLog())
        {
            if (isPrefix_)
            {
                spdlog::log(level_, "{}{}", leave_, message_);
            }
            else
            {
                spdlog::log(level_, "{}{}", message_, leave_);
            }
        }
    }
    catch (...)
    {
        std::cerr << "Failed to log scope message" << '\n';
    }
}
