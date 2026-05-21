#include <Windows.h>

#include <core/utils/SingleInstanceChecker.h>
#include <spdlog/spdlog.h>

namespace core {

SingleInstanceChecker::SingleInstanceChecker(std::wstring_view name)
    : appName_(name)
{
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
}

SingleInstanceChecker::~SingleInstanceChecker()
{
    if (mutex_ != 0)
    {
        const auto handle = reinterpret_cast<HANDLE>(mutex_);
        ReleaseMutex(handle);
        CloseHandle(handle);
        mutex_ = 0;
    }
}

} // namespace core
