#include <unistd.h>
#include <cerrno>
#include <cstring>

#include <core/utils/Sys.h>

namespace core::sys {

bool isUserInteractive() noexcept
{
    // No controlling terminal means we were launched by launchd (daemon mode)
    return isatty(STDIN_FILENO) != 0;
}

std::string errorDescription(uint64_t code)
{
    return std::strerror(static_cast<int>(code));
}

std::string constructLastErrorMsg(std::string_view message)
{
    return constructErrorMsg(message, static_cast<uint64_t>(errno));
}

void logLastError(const std::string_view message)
{
    logError(message, static_cast<uint64_t>(errno));
}

uint32_t currentProcessId() noexcept
{
    return static_cast<uint32_t>(getpid());
}

} // namespace core::sys
