#include <Windows.h>
#include <winuser.h>
#include <psapi.h>

#include "Window.h"

#include <fmt/format.h>

#include <array>

#pragma comment(lib, "Psapi.lib")

std::string hwndToString(HWND hwnd)
{
    uint64_t value = *reinterpret_cast<const uint64_t*>(&hwnd);

    return fmt::format("{:#010x}", value);
}

WindowImpl::WindowImpl(HWND hwnd) noexcept
    : hwnd_(hwnd)
    , id_(hwndToString(hwnd_))
{
}

const std::string& WindowImpl::id() const
{
    return id_;
}

std::string WindowImpl::title() const
{
    std::array<char, 256> buffer;
    const int size = GetWindowTextA(hwnd_, buffer.data(), static_cast<int>(buffer.size()));

    if (size)
    {
        return std::string(buffer.data(), size);
    }

    return {};
}

std::string WindowImpl::className() const
{
    std::array<char, 256> buffer;
    const int size = RealGetWindowClassA(hwnd_, buffer.data(), static_cast<int>(buffer.size()));

    if (size)
    {
        return std::string(buffer.data(), size);
    }

    return {};
}

std::string WindowImpl::ownerProcessPath() const
{
    DWORD procId{ 0 };
    GetWindowThreadProcessId(hwnd_, &procId);
    HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procId);

    if (nullptr == proc)
    {
        return {};
    }

    HMODULE modules{ nullptr };
    DWORD modulesCount{ 0 };
    DWORD sz{ 0 };
    std::array<char, 256> modulePath {};

    if (EnumProcessModules(proc, &modules, sizeof(modules), &modulesCount))
    {
        sz = GetModuleFileNameEx(proc, modules, modulePath.data(),
                                 static_cast<DWORD>(modulePath.size()));
    }

    CloseHandle(proc);

    return std::string(modulePath.data(), sz);
}

uint64_t WindowImpl::ownerProcessId() const noexcept
{
    DWORD procId{ 0 };
    GetWindowThreadProcessId(hwnd_, &procId);

    return procId;
}

Rect WindowImpl::boundingRect() const noexcept
{
    WINDOWPLACEMENT wndpl = {};

    if (GetWindowPlacement(hwnd_, &wndpl))
    {
        const RECT& rc = wndpl.rcNormalPosition;

        Point pt(rc.left, rc.top);
        Dimensions dims(rc.right - rc.left, rc.bottom - rc.top);

        return Rect(pt, dims);
    }

    return Rect();
}
