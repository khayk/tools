#include "Api.h"
#include "Window.h"
#include "ProcessLauncher.h"
#include "CoreGraphicsUtils.h"

#include <kidmon/geometry/Dimensions.h>
#include <kidmon/geometry/Point.h>

using namespace km;

namespace km {

ApiPtr ApiFactory::create()
{
    return std::make_unique<ApiImpl>();
}

} // namespace km

WindowPtr ApiImpl::foregroundWindow()
{
    const cg::WindowInfo info = cg::foregroundWindowInfo();
    if (!info.valid)
        return {};

    Point origin(info.originX, info.originY);
    Dimensions dims(info.width, info.height);

    return std::make_unique<WindowImpl>(info.id,
                                        static_cast<pid_t>(info.pid),
                                        info.title,
                                        info.ownerName,
                                        Rect(origin, dims));
}

ProcessLauncherPtr ApiImpl::createProcessLauncher()
{
    return std::make_unique<ProcessLauncherImpl>();
}
