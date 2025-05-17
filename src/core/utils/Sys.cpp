#ifdef _WIN32
    #include <Windows.h>
    #include <WtsApi32.h>

    #include <array>

    #pragma comment(lib, "Wtsapi32.lib")
#else
    #include <sys/types.h>
    #include <sys/stat.h>

    #include <fstream>
#endif

#include <core/utils/Sys.h>
#include <spdlog/spdlog.h>

namespace {

#ifdef _WIN32
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
#endif

} // namespace


namespace sys {

std::wstring activeUserName()
{
#ifdef _WIN32
    return userNameBySessionId(WTSGetActiveConsoleSessionId());
#else
    throw std::runtime_error(fmt::format("Not implemented: {}", __func__));
#endif
}

bool isUserInteractive() noexcept
{
    bool interactiveUser = true;

#ifdef _WIN32
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
#else

#endif

    return interactiveUser;
}

std::string errorDescription(uint64_t code)
{
    std::string message;

#ifdef _WIN32
    if (code == 0)
    {
        return message; /// No error message has been recorded
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

    if (size > 0)
    {
        /// get rid of \r\n
        message.assign(buffer.data(), std::min<size_t>(size, size - 2));
    }

    return message;
#else
    message = std::strerror(static_cast<int>(code));
    return message;
#endif
}

std::string constructErrorMsg(const std::string_view message, const uint64_t errorCode)
{
    if (errorCode == 0)
    {
        return std::string(message);
    }

    return fmt::format("{}, errorCode = {}, desc: {}",
                       message,
                       errorCode,
                       errorDescription(errorCode));
}

std::string constructLastErrorMsg(std::string_view message)
{
#ifdef _WIN32
    return constructErrorMsg(message, GetLastError());
#else
    return constructErrorMsg(message, static_cast<uint64_t>(errno));
#endif
}

void logError(const std::string_view message, const uint64_t errorCode)
{
    const std::string output = constructErrorMsg(message, errorCode);
    spdlog::error(output);
}

void logLastError(const std::string_view message)
{
#ifdef _WIN32
    logError(message, GetLastError());
#else
    logError(message, static_cast<uint64_t>(errno));
#endif
}

uint32_t currentProcessId() noexcept
{
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return static_cast<uint32_t>(getpid());
#endif // _WIN32
}

#ifdef _WIN32

#else

std::string getExecutablePathReadlink(int pid)
{
    // PATH_MAX is the POSIX defined maximum path length.
    // While not guaranteed to be sufficient in all edge cases,
    // it's typically large enough for /proc/self/exe.
    std::vector<char> buffer(PATH_MAX);

    // Read the symbolic link.
    const auto fileLink = fmt::format("/proc/{}/exe", pid);

    ssize_t count = readlink(fileLink.c_str(), buffer.data(), buffer.size());

    if (count == -1)
    {
        throw std::runtime_error(
            fmt::format("Failed to read {}: {}", fileLink, strerror(errno)));
    }

    // Check if the buffer might have been too small.
    // readlink doesn't null-terminate if the buffer is filled completely.
    if (static_cast<size_t>(count) >= buffer.size())
    {
        // This is less likely with PATH_MAX but technically possible.
        // A more robust solution might involve dynamically resizing the buffer and
        // retrying.
        throw std::runtime_error(
            "Executable path may have been truncated (PATH_MAX too small?)");
    }

    return std::string {buffer.data(), static_cast<size_t>(count)};
}

#endif // _WIN32

fs::path currentProcessPath()
{
#ifdef _WIN32

    std::array<wchar_t, MAX_PATH> buf {};

    DWORD sz = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (sz > 0)
    {
        return fs::path(buf.data(), buf.data() + sz).lexically_normal();
    }
#else
    auto pid = getpid();

    return getExecutablePathReadlink(pid);

#endif // _WIN32

    return {};
}

size_t processMemoryUsage(uint32_t pid)
{
#ifdef _WIN32
    std::ignore = pid;
    return 0;
#else
    std::string filename = fmt::format("/proc/{}/status", pid);
    std::ifstream file(filename, std::ios::in);

    if (!file.is_open())
    {
        return 0;
    }

    std::string line;
    size_t memory = 0;
    while (getline(file, line))
    {
        if (line.starts_with("VmRSS:"))
        {
            std::istringstream iss(line);
            std::string label;
            iss >> label >> memory;
            break;
        }
    }

    return memory * 1024;
#endif
}

size_t currentProcessMemoryUsage()
{
    return processMemoryUsage(currentProcessId());
}

} // namespace sys