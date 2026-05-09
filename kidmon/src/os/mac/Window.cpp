#include "Window.h"
#include <core/utils/Throw.h>

const std::string& WindowImpl::id() const
{
    return id_;
}

std::string WindowImpl::title() const
{
    core::throwNotImplemented();
    return {};
}

std::string WindowImpl::className() const
{
    core::throwNotImplemented();
    return {};
}

fs::path WindowImpl::ownerProcessPath() const
{
    core::throwNotImplemented();
    return {};
}

uint64_t WindowImpl::ownerProcessId() const
{
    core::throwNotImplemented();
    return 0;
}

Rect WindowImpl::boundingRect() const noexcept
{
    return {};
}

bool WindowImpl::capture(const ImageFormat format, std::vector<char>& content)
{
    core::throwNotImplemented();
    std::ignore = format;
    std::ignore = content;
    return false;
}
