#ifdef _WIN32
    #include <Windows.h>
#elif defined(__APPLE__)
    #include <cerrno>
    #include <cstring>
    #include <fcntl.h>
    #include <sys/file.h>
    #include <unistd.h>
#endif

#include <core/utils/SingleInstanceChecker.h>
#include <core/utils/Str.h>
#include <spdlog/spdlog.h>

namespace core {

SingleInstanceChecker::SingleInstanceChecker(std::wstring_view name)
    : appName_(name)
{
#ifdef _WIN32
    const auto handle = CreateMutexW(nullptr, TRUE, (L"Global\\" + appName_).data());
    const auto error = GetLastError();

    if (handle == nullptr)
    {
        spdlog::error("CreateMutex failed, ec: {}", error);
        processAlreadyRunning_ = true;
        return;
    }

    mutex_ = reinterpret_cast<std::intptr_t>(handle);
    processAlreadyRunning_ = (error == ERROR_ALREADY_EXISTS);
#elif defined(__APPLE__)
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
#else
    std::ignore = name;
    spdlog::error("Single instance checker is not implemented");
#endif
}

SingleInstanceChecker::~SingleInstanceChecker()
{
#ifdef _WIN32
    if (mutex_ != 0)
    {
        const auto handle = reinterpret_cast<HANDLE>(mutex_);
        ReleaseMutex(handle);
        CloseHandle(handle);
    }
#elif defined(__APPLE__)
    if (mutex_ != 0)
    {
        const int fd = static_cast<int>(mutex_ - 1);
        flock(fd, LOCK_UN);
        close(fd);
    }
#endif
    mutex_ = 0;
}

bool SingleInstanceChecker::processAlreadyRunning() const noexcept
{
    return processAlreadyRunning_;
}

void SingleInstanceChecker::report() const
{
    spdlog::info("One instance of '{}' is already running.", str::ws2s(appName_));
}

} // namespace core
