#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <core/utils/SingleInstanceChecker.h>
#include <core/utils/Str.h>
#include <spdlog/spdlog.h>

namespace core {

SingleInstanceChecker::SingleInstanceChecker(std::wstring_view name)
    : appName_(name)
{
    const auto lockPath = "/tmp/" + str::ws2s(name) + ".lock";

    // open() accepts a variadic mode argument per the POSIX spec — no C++ alternative.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    const int fd = open(lockPath.c_str(), O_CREAT | O_RDWR, 0666);

    if (fd < 0)
    {
        spdlog::error("Failed to open lock file '{}': {}", lockPath, std::strerror(errno));
        processAlreadyRunning_ = true;
        return;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) != 0)
    {
        processAlreadyRunning_ = true;
        if (errno != EWOULDBLOCK)
        {
            spdlog::error("flock failed for '{}': {}", lockPath, std::strerror(errno));
        }
        close(fd);
        return;
    }

    // Store fd + 1 so that 0 remains the "no handle" sentinel (fd 0 = stdin is valid).
    mutex_ = static_cast<std::intptr_t>(fd) + 1;
}

SingleInstanceChecker::~SingleInstanceChecker()
{
    if (mutex_ != 0)
    {
        const int fd = static_cast<int>(mutex_ - 1);
        flock(fd, LOCK_UN);
        close(fd);
        mutex_ = 0;
    }
}

} // namespace core
