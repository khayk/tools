#include <core/utils/Sys.h>
#include <spdlog/spdlog.h>

namespace core::sys {

std::string constructErrorMsg(const std::string_view message, const uint64_t errorCode)
{
    if (errorCode == 0)
    {
        return std::string(message);
    }

    return std::format("{}, errorCode = {}, desc: {}",
                       message,
                       errorCode,
                       errorDescription(errorCode));
}

void logError(const std::string_view message, const uint64_t errorCode)
{
    spdlog::error(constructErrorMsg(message, errorCode));
}

size_t currentProcessMemoryUsage()
{
    return processMemoryUsage(currentProcessId());
}

} // namespace core::sys
