#include "ProcessLauncher.h"
#include <core/utils/Throw.h>

bool ProcessLauncherImpl::launch(const fs::path& exec,
                                 const std::vector<std::string>& args)
{
    core::throwNotImplemented();
    std::ignore = exec;
    std::ignore = args;

    return false;
}