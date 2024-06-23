#include "ProcessLauncher.h"
#include <kidmon/common/Utils.h>
#include <kidmon/common/Tracer.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Str.h>

#include <Windows.h>
#include <UserEnv.h>
#include <WtsApi32.h>

#include <spdlog/spdlog.h>

#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "Wtsapi32.lib")

namespace {

struct HandleCloser
{
    using pointer = HANDLE;
    void operator()(const HANDLE h) const
    {
        if (h != nullptr)
        {
            CloseHandle(h);
        }
    }
};

using HandleUPtr = std::unique_ptr<HANDLE, HandleCloser>;

HandleUPtr activeUserQueryToken()
{
    static unsigned int previousSessionId = 0;
    const unsigned int sessionId = WTSGetActiveConsoleSessionId();

    if (previousSessionId != sessionId)
    {
        spdlog::debug("Active console session id: {}", sessionId);
        previousSessionId = sessionId;
    }

    HANDLE token {nullptr};
    if (!WTSQueryUserToken(sessionId, &token))
    {
        const auto errorCode = GetLastError();
        spdlog::warn("WTSQueryUserToken failed, errorCode: {}, desc: {}",
                     errorCode,
                     sys::errorDescription(errorCode));
        return {};
    }

    return HandleUPtr {token};
}


HandleUPtr activeUserMaxAllowedToken()
{
    const HandleUPtr userToken = activeUserQueryToken();

    if (!userToken)
    {
        return {};
    }

    spdlog::debug("Active user token: {}", fmt::ptr(userToken.get()));

    // Get the linked token
    TOKEN_LINKED_TOKEN tempLinkedToken {};
    unsigned long size = sizeof(tempLinkedToken);
    const BOOL ret = GetTokenInformation(userToken.get(),
                                         TokenLinkedToken,
                                         static_cast<void*>(&tempLinkedToken),
                                         size,
                                         &size);
    if (!ret)
    {
        spdlog::debug(sys::constructLastErrorMsg(
            fmt::format("GetTokenInformation failed for token class: {}",
                        static_cast<int>(TokenLinkedToken))));
    }

    const HandleUPtr linkedToken(tempLinkedToken.LinkedToken);

    HANDLE token {nullptr};
    if (!DuplicateTokenEx(linkedToken.get() ? linkedToken.get() : userToken.get(),
                          MAXIMUM_ALLOWED,
                          nullptr,
                          SecurityImpersonation,
                          TokenPrimary,
                          &token))
    {
        spdlog::debug(sys::constructLastErrorMsg("DuplicateTokenEx failed"));
        return {};
    }

    return HandleUPtr {token};
}


bool directLaunch(const fs::path& exec, const std::vector<std::string>& args)
{
    STARTUPINFOW startupInfo{};
    PROCESS_INFORMATION processInfo;
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    memset(&processInfo, 0, sizeof(PROCESS_INFORMATION));

    startupInfo.dwFlags = STARTF_FORCEOFFFEEDBACK;
    const DWORD creationFlags =
        GetConsoleWindow() ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW;

    std::wstring cl {};
    for (const auto& e : args)
    {
        cl += L" ";
        cl += str::s2ws(e);
    }

    spdlog::trace("Executing command: {}{}", exec, str::ws2s(cl));

    const auto result = CreateProcessW(exec.wstring().c_str(),
                                       cl.data(),
                                       nullptr,
                                       nullptr,
                                       false,
                                       creationFlags,
                                       nullptr,
                                       nullptr,
                                       &startupInfo,
                                       &processInfo);
    if (!result)
    {
        sys::logLastError("CreateProcess failed for '" + file::path2s(exec) + "'");
        return false;
    }

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return true;
}


bool interactiveLaunch(const fs::path& exec,
                       const std::vector<std::string>& args,
                       HandleUPtr token)
{
    ScopedTrace tracer {__FUNCTION__};

    if (!token)
    {
        return false;
    }

    void* ptr = nullptr;
    if (!CreateEnvironmentBlock(&ptr, token.get(), FALSE))
    {
        return false;
    }

    const auto envBlockDeleter = [](auto* envPtr) noexcept {
        DestroyEnvironmentBlock(envPtr);
    };
    const std::unique_ptr<void, decltype(envBlockDeleter)> envBlock(ptr, envBlockDeleter);

    DWORD creationFlags = NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;
    std::wstring name = L"winsta0\\default";

    STARTUPINFOW startupInfo {};
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.lpDesktop = name.data();
    startupInfo.dwFlags = STARTF_FORCEOFFFEEDBACK;
    creationFlags |= (GetConsoleWindow() ? 0 : CREATE_NO_WINDOW);

    PROCESS_INFORMATION processInfo;
    ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

    std::wstring cmdLine = L"\"" + exec.wstring() + L"\"";
    for (const auto& arg : args)
    {
        cmdLine += L" ";
        cmdLine += str::s2ws(arg);
    }

    spdlog::trace("Executing command: " + str::ws2s(cmdLine));

    const auto ret = CreateProcessAsUserW(token.get(),
                                          nullptr,
                                          cmdLine.data(),
                                          nullptr,
                                          nullptr,
                                          FALSE,
                                          creationFlags,
                                          envBlock.get(),
                                          nullptr,
                                          &startupInfo,
                                          &processInfo);
    if (!ret)
    {
        sys::logLastError("CreateProcessAsUserW failed");
        return false;
    }

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return true;
}

} // namespace


bool ProcessLauncherImpl::launch(const fs::path& exec,
                                 const std::vector<std::string>& args)
{
    auto token = activeUserMaxAllowedToken();

    if (token)
    {
        return interactiveLaunch(exec, args, std::move(token));
    }

    return directLaunch(exec, args);
}