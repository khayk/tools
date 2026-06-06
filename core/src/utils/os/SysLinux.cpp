#include <unistd.h>
#include <pwd.h>
#include <climits>
#include <cstdlib>
#include <format>
#include <cerrno>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <core/utils/Sys.h>
#include <core/utils/Str.h>

namespace {

std::string getExecutablePathReadlink(int pid)
{
    // PATH_MAX is the POSIX defined maximum path length.
    // While not guaranteed to be sufficient in all edge cases,
    // it's typically large enough for /proc/self/exe.
    std::vector<char> buffer(PATH_MAX);

    const auto fileLink = std::format("/proc/{}/exe", pid);
    const ssize_t count = readlink(fileLink.c_str(), buffer.data(), buffer.size());

    if (count == -1)
    {
        throw std::runtime_error(
            std::format("Failed to read {}: {}", fileLink, strerror(errno)));
    }

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

} // namespace

namespace core::sys {

std::wstring activeUserName()
{
    // Resolve the login name of the user owning this process. getpwuid_r on the
    // real UID is the most reliable source; fall back to the common environment
    // variables when /etc/passwd lookup is unavailable (e.g. minimal containers).
    const uid_t uid = getuid();

    long bufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufSize <= 0)
    {
        bufSize = 16'384; // generous default when the limit is indeterminate
    }

    std::vector<char> buffer(static_cast<size_t>(bufSize));
    passwd pwd {};
    passwd* result = nullptr;

    if (getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &result) == 0 &&
        result != nullptr && (pwd.pw_name != nullptr) && (pwd.pw_name[0] != '\0'))
    {
        return core::str::s2ws(std::string_view(pwd.pw_name));
    }

    for (const char* var : {"USER", "LOGNAME"})
    {
        if (const char* name = std::getenv(var);
            (name != nullptr) && (name[0] != '\0'))
        {
            return core::str::s2ws(std::string_view(name));
        }
    }

    throw std::runtime_error("Unable to determine active user name");
}

fs::path currentProcessPath()
{
    return getExecutablePathReadlink(getpid());
}

size_t processMemoryUsage(uint32_t pid)
{
    const std::string filename = std::format("/proc/{}/status", pid);
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
}

} // namespace core::sys
