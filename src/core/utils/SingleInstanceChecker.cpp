#ifdef _WIN32
    #include <Windows.h>
#else
#endif

#include <core/utils/SingleInstanceChecker.h>
#include <core/utils/Str.h>
#include <spdlog/spdlog.h>

SingleInstanceChecker::SingleInstanceChecker(std::wstring_view name)
    : appName_(name)
{
#ifdef _WIN32
    mutex_ = CreateMutexW(nullptr, TRUE, (L"Global\\" + appName_).data());
    const auto error = GetLastError();

    if (mutex_ == nullptr)
    {
        spdlog::error("CreateMutex failed, ec: {}", error);
        processAlreadyRunning_ = true;

        return;
    }

    processAlreadyRunning_ = (error == ERROR_ALREADY_EXISTS);
#else
    std::ignore = name;
    spdlog::error("Single instance checker is not implemented");
#endif
}

SingleInstanceChecker::~SingleInstanceChecker()
{
#ifdef _WIN32
    if (mutex_)
    {
        ReleaseMutex(mutex_);
        CloseHandle(mutex_);
    }
#else
#endif
    mutex_ = nullptr;
}

bool SingleInstanceChecker::processAlreadyRunning() const noexcept
{
    return processAlreadyRunning_;
}

void SingleInstanceChecker::report() const
{
    spdlog::info("One instance of '{}' is already running.", str::ws2s(appName_));
}
