#include <Windows.h>

#include "Api.h"
#include "Window.h"
#include "ProcessLauncher.h"

#include <spdlog/spdlog.h>

ApiPtr ApiFactory::create()
{
    return std::make_unique<ApiImpl>();
}

WindowPtr ApiImpl::foregroundWindow()
{
    HWND hwnd = GetForegroundWindow();

    if (hwnd != nullptr)
    {
        return std::make_unique<WindowImpl>(hwnd);
    }

    return WindowPtr();
}

ProcessLauncherPtr ApiImpl::createProcessLauncher()
{
    return std::make_unique<ProcessLauncherImpl>();
}
