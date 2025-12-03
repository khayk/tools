#include "ProcessLauncher.h"
#include <fmt/format.h>

bool ProcessLauncherImpl::launch(const fs::path& exec,
                                 const std::vector<std::string>& args)
{
    throw std::logic_error(fmt::format("Not implemented: {}", __func__));
    std::ignore = exec;
    std::ignore = args;

    return false;
}