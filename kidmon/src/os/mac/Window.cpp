#include "Window.h"
#include "CoreGraphicsUtils.h"

#include <kidmon/geometry/Dimensions.h>
#include <kidmon/geometry/Point.h>

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
    return cg::processPath(static_cast<int32_t>(pid_));
}

uint64_t WindowImpl::ownerProcessId() const
{
    return static_cast<uint64_t>(pid_);
}

Rect WindowImpl::boundingRect() const noexcept
{
    return rect_;
}

bool WindowImpl::capture(const ImageFormat format, std::vector<char>& content)
{
    return cg::captureWindow(windowId_, format, content);
}
