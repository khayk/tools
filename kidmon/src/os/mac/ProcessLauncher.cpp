#include "ProcessLauncher.h"

// note: whole file is claude generated
#include <core/utils/Sys.h>
#include <core/utils/Tracer.h>

#include <SystemConfiguration/SystemConfiguration.h>
#include <pwd.h>
#include <sys/wait.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

namespace {

uid_t activeConsoleUserUid()
{
    uid_t uid {};
    CFStringRef cfName = SCDynamicStoreCopyConsoleUser(nullptr, &uid, nullptr);
    if (cfName != nullptr)
    {
        CFRelease(cfName);
        return uid;
    }
    return static_cast<uid_t>(-1);
}

struct Argv
{
    std::vector<std::string> strings;
    std::vector<char*> ptrs;
};

Argv buildArgv(const fs::path& exec, const std::vector<std::string>& args)
{
    Argv result;
    result.strings.reserve(args.size() + 1);
    result.strings.push_back(exec.string());
    for (const auto& arg : args)
    {
        result.strings.push_back(arg);
    }

    result.ptrs.reserve(result.strings.size() + 1);
    for (auto& s : result.strings)
    {
        result.ptrs.push_back(s.data());
    }
    result.ptrs.push_back(nullptr);

    return result;
}

// Double-fork so the grandchild is adopted by launchd (no zombie).
bool forkAndExec(const fs::path& exec, const std::vector<std::string>& args)
{
    auto argv = buildArgv(exec, args);

    const pid_t child = fork();
    if (child < 0)
    {
        core::sys::logLastError("fork failed");
        return false;
    }

    if (child == 0)
    {
        const pid_t grandchild = fork();
        if (grandchild != 0)
        {
            _exit(grandchild > 0 ? 0 : 1);
        }

        execv(exec.c_str(), argv.ptrs.data());
        _exit(1);
    }

    waitpid(child, nullptr, 0);
    return true;
}

bool directLaunch(const fs::path& exec, const std::vector<std::string>& args)
{
    spdlog::trace("Executing command: {}", exec.string());
    return forkAndExec(exec, args);
}

bool interactiveLaunch(const fs::path& exec,
                       const std::vector<std::string>& args,
                       uid_t uid)
{
    ScopedTrace tracer {__FUNCTION__};

    struct passwd* pw = getpwuid(uid);
    if (pw == nullptr)
    {
        spdlog::error("getpwuid failed for uid {}", uid);
        return false;
    }

    const gid_t gid = pw->pw_gid;
    auto argv = buildArgv(exec, args);

    spdlog::trace("Executing command: {} as uid={}", exec.string(), uid);

    const pid_t child = fork();
    if (child < 0)
    {
        core::sys::logLastError("fork failed");
        return false;
    }

    if (child == 0)
    {
        const pid_t grandchild = fork();
        if (grandchild != 0)
        {
            _exit(grandchild > 0 ? 0 : 1);
        }

        if (setgid(gid) != 0 || setuid(uid) != 0)
        {
            _exit(1);
        }

        execv(exec.c_str(), argv.ptrs.data());
        _exit(1);
    }

    waitpid(child, nullptr, 0);
    return true;
}

} // namespace


bool ProcessLauncherImpl::launch(const fs::path& exec,
                                 const std::vector<std::string>& args)
{
    const uid_t uid = activeConsoleUserUid();

    if (uid != static_cast<uid_t>(-1))
    {
        return interactiveLaunch(exec, args, uid);
    }

    return directLaunch(exec, args);
}
