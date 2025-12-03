#include "Api.h"
#include "Window.h"
#include "ProcessLauncher.h"

#include <fmt/format.h>

ApiPtr ApiFactory::create()
{
    return std::make_unique<ApiImpl>();
}

WindowPtr ApiImpl::foregroundWindow()
{
    throw std::logic_error(fmt::format("Not implemented: {}", __func__));
    return WindowPtr {};
}

ProcessLauncherPtr ApiImpl::createProcessLauncher()
{
    return std::make_unique<ProcessLauncherImpl>();
}
