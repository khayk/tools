#include "Api.h"
#include "Window.h"
#include "ProcessLauncher.h"

#include <exception>

ApiPtr ApiFactory::create()
{
    return std::make_unique<ApiImpl>();
}

WindowPtr ApiImpl::foregroundWindow()
{
    throw std::logic_error("Not implemented");
    return WindowPtr();
}

ProcessLauncherPtr ApiImpl::createProcessLauncher()
{
    return std::make_unique<ProcessLauncherImpl>();
}
