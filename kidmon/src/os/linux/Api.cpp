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

std::chrono::milliseconds ApiImpl::idleTime()
{
    // Not implemented for Linux yet (would need X11 XScreenSaver / a Wayland
    // idle protocol). Reporting zero keeps the user always "active", matching
    // the not-yet-implemented foregroundWindow above.
    return std::chrono::milliseconds::zero();
}

bool ApiImpl::displaySleepPrevented()
{
    // Not implemented for Linux yet (would query an idle-inhibit protocol). The
    // user is already always treated as active here, so this is moot for now.
    return false;
}
