#include <unistd.h>
#include <climits>
#include <format>
#include <cerrno>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <core/utils/Sys.h>
#include <core/utils/Throw.h>

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
    core::throwNotImplemented();
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
