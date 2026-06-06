#include "ProcessLauncher.h"
#include <core/utils/FmtExt.h>
#include <core/utils/Str.h>
#include <core/utils/Sys.h>
#include <core/utils/Tracer.h>


#include <Windows.h>
#include <UserEnv.h>
#include <WtsApi32.h>

#include <spdlog/spdlog.h>

#include <cwchar>
#include <vector>

#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "Wtsapi32.lib")

using namespace core;
namespace {

// Produce a CREATE_UNICODE_ENVIRONMENT block (double-null terminated) from an
// existing environment block, appending the extra variables and dropping any
// inherited entry they override. Lets us inject secrets (e.g. the auth token)
// into the child's environment instead of putting them on its command line.
std::vector<wchar_t> mergeEnvBlock(const wchar_t* base, const km::Env& env)
{
    std::vector<std::wstring> extra;
    std::vector<std::wstring> extraKeys;
    extra.reserve(env.size());
    extraKeys.reserve(env.size());
    for (const auto& [k, v] : env)
    {
        extra.push_back(str::s2ws(k) + L"=" + str::s2ws(v));
        extraKeys.push_back(str::s2ws(k));
    }

    const auto overridden = [&extraKeys](const wchar_t* entry, size_t len) {
        const wchar_t* eq = static_cast<const wchar_t*>(wmemchr(entry, L'=', len));
        const size_t keyLen = eq ? static_cast<size_t>(eq - entry) : len;
        for (const auto& key : extraKeys)
        {
            // Windows environment variable names are case-insensitive.
            if (key.size() == keyLen && _wcsnicmp(key.c_str(), entry, keyLen) == 0)
            {
                return true;
            }
        }
        return false;
    };

    std::vector<wchar_t> out;
    for (const wchar_t* p = base; p != nullptr && *p != L'\0';)
    {
        const size_t len = wcslen(p);
        if (!overridden(p, len))
        {
            out.insert(out.end(), p, p + len + 1); // include the null terminator
        }
        p += len + 1;
    }

    for (const auto& e : extra)
    {
        out.insert(out.end(), e.c_str(), e.c_str() + e.size() + 1);
    }

    out.push_back(L'\0'); // final block terminator
    return out;
}


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
            std::format("GetTokenInformation failed for token class: {}",
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


bool directLaunch(const fs::path& exec,
                  const km::Args& args,
                  const km::Env& env)
{
    STARTUPINFOW startupInfo {};
    PROCESS_INFORMATION processInfo;
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    memset(&processInfo, 0, sizeof(PROCESS_INFORMATION));

    startupInfo.dwFlags = STARTF_FORCEOFFFEEDBACK;
    const DWORD creationFlags =
        (GetConsoleWindow() ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW) |
        CREATE_UNICODE_ENVIRONMENT;

    // Inherit the current environment, plus any injected variables. RAII frees
    // the source block on every exit path, including if the merge below throws.
    LPWCH baseEnv = GetEnvironmentStringsW();
    const auto envStringsDeleter = [](LPWCH p) noexcept {
        if (p != nullptr)
        {
            FreeEnvironmentStringsW(p);
        }
    };
    const std::unique_ptr<wchar_t, decltype(envStringsDeleter)> baseEnvGuard(
        baseEnv,
        envStringsDeleter);

    std::vector<wchar_t> envBlock = mergeEnvBlock(baseEnv, env);

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
                                       envBlock.data(),
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
                       const km::Args& args,
                       const km::Env& env,
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

    // RAII so the user's environment block is freed on every exit path,
    // including if the merge below throws (mergeEnvBlock allocates).
    const auto envBlockDeleter = [](void* envPtr) noexcept {
        DestroyEnvironmentBlock(envPtr);
    };
    const std::unique_ptr<void, decltype(envBlockDeleter)> userEnv(ptr,
                                                                   envBlockDeleter);

    // Merge the injected variables into a private copy passed to the child.
    std::vector<wchar_t> envBlock =
        mergeEnvBlock(static_cast<const wchar_t*>(ptr), env);

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
                                          envBlock.data(),
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
                                 const km::Args& args,
                                 const km::Env& env)
{
    auto token = activeUserMaxAllowedToken();

    if (token)
    {
        return interactiveLaunch(exec, args, env, std::move(token));
    }

    return directLaunch(exec, args, env);
}