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

bool ApiImpl::displayOn()
{
    // Not implemented for Linux yet (would query the display power state / an
    // idle-inhibit protocol). Reporting the display as on keeps the user always
    // treated as present, matching the not-yet-implemented idleTime above.
    return true;
}
