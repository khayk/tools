#ifdef _WIN32
    #include <Shlobj.h>
    #include <Knownfolders.h>
    #include <WtsApi32.h>

    #pragma comment(lib, "Wtsapi32.lib")
#else
#endif

#include "Utils.h"

#include <utils/Str.h>
#include <utils/FmtExt.h>

#include <spdlog/spdlog.h>

#include <system_error>
#include <filesystem>
#include <array>

namespace fs = std::filesystem;

namespace sys {

std::wstring userNameBySessionId(unsigned long sessionId)
{
#ifdef _WIN32
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
#else
    throw std::runtime_error("userNameBySessionId not implemented");
#endif
}

std::wstring activeUserName()
{
#ifdef _WIN32
    return userNameBySessionId(WTSGetActiveConsoleSessionId());
#else
    // @todo:khayk
    throw std::runtime_error("activeUserName not implemented");
#endif
}

bool isUserInteractive() noexcept
{
    bool interactiveUser = true;

    HWINSTA hWinStation = GetProcessWindowStation();

    if (hWinStation != nullptr)
    {
        USEROBJECTFLAGS uof = {0};
        if (GetUserObjectInformation(hWinStation,
                                     UOI_FLAGS,
                                     &uof,
                                     sizeof(USEROBJECTFLAGS),
                                     NULL) &&
            ((uof.dwFlags & WSF_VISIBLE) == 0))
        {
            interactiveUser = false;
        }
    }

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
    message = std::strerror(code);
    return message;
#endif
}

std::string constructMessage(const std::string_view message, const uint64_t errorCode)
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

void logError(const std::string_view message, const uint64_t errorCode) noexcept
{
    try
    {
        const std::string output = constructMessage(message, errorCode);

        // We should not report these errors by default
        spdlog::error(output);
    }
    catch (const std::exception& ex)
    {
        std::ignore = ex;
    }
}

void logLastError(const std::string_view message) noexcept
{
    logError(message, GetLastError());
}

} // namespace sys


namespace dirs {

#ifdef _WIN32
fs::path getKnownFolderPath(const GUID& id, std::error_code& ec) noexcept
{
    constexpr DWORD flags = KF_FLAG_CREATE;
    HANDLE token = nullptr;
    PWSTR dest = nullptr;

    const HRESULT result = SHGetKnownFolderPath(id, flags, token, &dest);
    fs::path path;

    if (result != S_OK)
    {
        ec.assign(GetLastError(), std::system_category());
    }
    else
    {
        path = dest;
        CoTaskMemFree(dest);
    }

    return path;
}
#else

#endif
fs::path home(std::error_code& ec)
{
#ifdef _WIN32
    return getKnownFolderPath(FOLDERID_Profile, ec);
#else
    throw std::logic_error("Not implemented");
#endif
}

fs::path home()
{
    std::error_code ec;
    auto str = home(ec);

    if (ec)
    {
        throw std::system_error(ec, "Failed to retrieve home directory");
    }

    return str;
}

fs::path temp(std::error_code& ec)
{
    fs::path path;

#ifdef _WIN32
    constexpr uint32_t bufferLength = MAX_PATH + 1;
    WCHAR buffer[bufferLength];
    auto length = GetTempPathW(bufferLength, buffer);

    if (length == 0)
    {
        ec.assign(GetLastError(), std::system_category());
    }
    else
    {
        path.assign(std::wstring_view(buffer, length));
    }
#else
    throw std::logic_error("Not implemented");
#endif

    return path;
}

fs::path temp()
{
    std::error_code ec;
    auto path = temp(ec);

    if (ec)
    {
        throw std::system_error(ec, "Failed to retrieve temp directory");
    }

    return path;
}

fs::path data(std::error_code& ec)
{
#ifdef _WIN32
    return getKnownFolderPath(FOLDERID_LocalAppData, ec);
#else
    throw std::logic_error("Not implemented");
#endif
}

fs::path data()
{
    std::error_code ec;
    auto path = data(ec);

    if (ec)
    {
        throw std::system_error(ec,
                                "Failed to retrieve local application data directory");
    }

    return path;
}

fs::path config(std::error_code& ec)
{
#ifdef _WIN32
    return getKnownFolderPath(FOLDERID_ProgramData, ec);
#else
    throw std::logic_error("Not implemented");
#endif
}

fs::path config()
{
    std::error_code ec;
    auto path = config(ec);

    if (ec)
    {
        throw std::system_error(ec, "Failed to retrieve program data directory");
    }

    return path;
}
} // namespace dirs

SingleInstanceChecker::SingleInstanceChecker(std::wstring_view name)
    : appName_(name)
{
    mutex_ = CreateMutexW(nullptr, TRUE, (L"Global\\" + appName_).data());
    const auto error = GetLastError();
    
    if (mutex_ == nullptr)
    {
        spdlog::error("CreateMutex failed, ec: {}", error);
        processAlreadyRunning_ = true;

        return;
    }

    processAlreadyRunning_ = (error == ERROR_ALREADY_EXISTS);
}

SingleInstanceChecker::~SingleInstanceChecker()
{
    if (mutex_)
    {
        ReleaseMutex(mutex_);
        CloseHandle(mutex_);
        mutex_ = nullptr;
    }
}

bool SingleInstanceChecker::processAlreadyRunning() const noexcept
{
    return processAlreadyRunning_;
}

void SingleInstanceChecker::report() const
{
    spdlog::info("One instance of '{}' is already running.", str::ws2s(appName_));
}
