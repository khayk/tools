#include <Windows.h>

#include "Api.h"
#include "Window.h"
#include "ProcessLauncher.h"

#include <spdlog/spdlog.h>

using namespace km;

namespace km {

ApiPtr ApiFactory::create()
{
    return std::make_unique<ApiImpl>();
}

} // namespace km

WindowPtr ApiImpl::foregroundWindow()
{
    HWND hwnd = GetForegroundWindow();

    if (hwnd != nullptr)
    {
        return std::make_unique<WindowImpl>(hwnd);
    }

    return {};
}

ProcessLauncherPtr ApiImpl::createProcessLauncher()
{
    return std::make_unique<ProcessLauncherImpl>();
}
