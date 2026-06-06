#include "ProcessLauncher.h"
#include <core/utils/Throw.h>

bool ProcessLauncherImpl::launch(const fs::path& exec,
                                 const km::Args& args,
                                 const km::Env& env)
{
    core::throwNotImplemented();
    std::ignore = exec;
    std::ignore = args;
    std::ignore = env;

    return false;
}