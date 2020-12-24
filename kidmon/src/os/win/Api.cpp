#include <Windows.h>

#include "Api.h"
#include "Window.h"

#include <spdlog/spdlog.h>

#include <array>

ApiPtr ApiFactory::create()
{
    return std::make_unique<ApiImpl>();
}

WindowPtr ApiImpl::forgroundWindow()
{
    HWND hwnd = GetForegroundWindow();

    if (hwnd != nullptr)
    {
        return std::make_unique<WindowImpl>(hwnd);
    }

    return WindowPtr();
}
