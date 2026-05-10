#include "Api.h"
#include "Window.h"
#include "ProcessLauncher.h"

#include <core/utils/Throw.h>

using namespace km;

namespace km {

ApiPtr ApiFactory::create()
{
    return std::make_unique<ApiImpl>();
}

} // namespace km

WindowPtr ApiImpl::foregroundWindow()
{
    core::throwNotImplemented();
    return WindowPtr {};
}

ProcessLauncherPtr ApiImpl::createProcessLauncher()
{
    return std::make_unique<ProcessLauncherImpl>();
}
