#include <Windows.h>
#include <WtsApi32.h>
#include <psapi.h>
#include <array>

#include <core/utils/Sys.h>
#include <spdlog/spdlog.h>

namespace {

std::wstring userNameBySessionId(unsigned long sessionId)
{
    wchar_t* buf = nullptr;
    DWORD bufLen = 0;

    if (!WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE,
                                     sessionId,
                                     WTSUserName,
                                     &buf,
                                     &bufLen))
    {
        spdlog::error("WTSQuerySessionInformationW failed");
        return {};
    }

    std::wstring str(buf);
    WTSFreeMemory(buf);
    return str;
}

} // namespace

namespace core::sys {

std::wstring activeUserName()
{
    return userNameBySessionId(WTSGetActiveConsoleSessionId());
}

bool isUserInteractive() noexcept
{
    bool interactiveUser = true;

    HWINSTA hWinStation = GetProcessWindowStation();
    if (hWinStation != nullptr)
    {
        USEROBJECTFLAGS uof = {};
        if (GetUserObjectInformation(hWinStation,
                                     UOI_FLAGS,
                                     &uof,
                                     sizeof(USEROBJECTFLAGS),
                                     nullptr) &&
            ((uof.dwFlags & WSF_VISIBLE) == 0))
        {
            interactiveUser = false;
        }
    }

    return interactiveUser;
}

std::string errorDescription(uint64_t code)
{
    if (code == 0)
    {
        return {}; // No error message has been recorded
    }

    std::array<CHAR, 512> buffer {};
    const size_t size =
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr,
                       static_cast<DWORD>(code),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       buffer.data(),
                       static_cast<DWORD>(buffer.size()),
                       nullptr);

    std::string message;
    if (size > 0)
    {
        // Strip trailing \r\n
        message.assign(buffer.data(), std::min<size_t>(size, size - 2));
    }
    return message;
}

std::string constructLastErrorMsg(std::string_view message)
{
    return constructErrorMsg(message, GetLastError());
}

void logLastError(const std::string_view message)
{
    logError(message, GetLastError());
}

uint32_t currentProcessId() noexcept
{
    return GetCurrentProcessId();
}

fs::path currentProcessPath()
{
    std::array<wchar_t, MAX_PATH> buf {};
    const DWORD sz =
        GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (sz > 0)
    {
        return fs::path(buf.data(), buf.data() + sz).lexically_normal();
    }
    return {};
}

size_t processMemoryUsage(uint32_t pid)
{
    const HANDLE hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, static_cast<DWORD>(pid));

    if (hProcess == nullptr)
    {
        return 0;
    }

    PROCESS_MEMORY_COUNTERS pmc {};
    size_t memory = 0;

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
    {
        memory = static_cast<size_t>(pmc.WorkingSetSize);
    }

    CloseHandle(hProcess);
    return memory;
}

} // namespace core::sys
