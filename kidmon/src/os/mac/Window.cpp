#include "Window.h"

#include <kidmon/geometry/Dimensions.h>
#include <kidmon/geometry/Point.h>

#include <libproc.h>
#include <spdlog/spdlog.h>

#include <array>
#include <format>

using namespace km;

WindowImpl::WindowImpl(uint32_t windowId,
                       pid_t pid,
                       std::string title,
                       std::string ownerName,
                       Rect rect) noexcept
    : windowId_(windowId)
    , pid_(pid)
    , id_(std::format("{:#010x}", windowId))
    , title_(std::move(title))
    , ownerName_(std::move(ownerName))
    , rect_(rect)
{
}

const std::string& WindowImpl::id() const
{
    return id_;
}

std::string WindowImpl::title() const
{
    return title_;
}

// macOS has no equivalent to Win32 window class names. The owner application
// name (kCGWindowOwnerName) is the closest useful substitute for monitoring.
std::string WindowImpl::className() const
{
    return ownerName_;
}

fs::path WindowImpl::ownerProcessPath() const
{
    std::array<char, PROC_PIDPATHINFO_MAXSIZE> buf {};
    if (proc_pidpath(static_cast<pid_t>(pid_), buf.data(), buf.size()) > 0)
    {
        return {buf.data()};
    }
    return {};
}

uint64_t WindowImpl::ownerProcessId() const
{
    return static_cast<uint64_t>(pid_);
}

Rect WindowImpl::boundingRect() const noexcept
{
    return rect_;
}

bool WindowImpl::capture([[maybe_unused]] const ImageFormat format,
                         [[maybe_unused]] std::vector<char>& content)
{
    // Per-window screen capture was removed from CoreGraphics in macOS 15.
    // The replacement (ScreenCaptureKit) is Objective-C only. Implementing
    // this would require a .mm compilation unit. windowId_ will be needed there.
    spdlog::warn("capture: not implemented for window {:#010x} on macOS (requires ScreenCaptureKit)",
                 windowId_);
    return false;
}
