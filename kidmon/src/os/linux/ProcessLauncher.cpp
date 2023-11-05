#include "ProcessLauncher.h"

bool ProcessLauncherImpl::launch(const fs::path& exec,
                                 const std::vector<std::string>& args)
{
    throw std::logic_error("Not implemented");
    std::ignore = exec;
    std::ignore = args;

    return false;
}