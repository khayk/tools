#include <SystemConfiguration/SystemConfiguration.h>
#include <libproc.h>
#include <unistd.h>
#include <array>
#include <stdexcept>

#include <core/utils/Sys.h>
#include <core/utils/Str.h>

namespace core::sys {

std::wstring activeUserName()
{
    CFStringRef cfName = SCDynamicStoreCopyConsoleUser(nullptr, nullptr, nullptr);
    if (cfName == nullptr)
    {
        throw std::runtime_error("No active console user");
    }
    std::array<char, 256> buf {};
    CFStringGetCString(cfName,
                       buf.data(),
                       static_cast<CFIndex>(buf.size()),
                       kCFStringEncodingUTF8);
    CFRelease(cfName);
    return core::str::s2ws(std::string_view(buf.data()));
}

fs::path currentProcessPath()
{
    pid_t pid = getpid();
    std::array<char, PROC_PIDPATHINFO_MAXSIZE> buf {};

    int ret = proc_pidpath(pid, buf.data(), buf.size());
    if (ret <= 0)
    {
        return {};
    }
    return fs::path {std::string(buf.data())};
}

size_t processMemoryUsage(uint32_t pid)
{
    struct proc_taskinfo info {};
    const int ret =
        proc_pidinfo(static_cast<int>(pid), PROC_PIDTASKINFO, 0, &info, sizeof(info));
    return ret > 0 ? static_cast<size_t>(info.pti_resident_size) : 0;
}

} // namespace core::sys
