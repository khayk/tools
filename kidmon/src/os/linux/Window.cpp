#include "Window.h"

using namespace km;

const std::string& WindowImpl::id() const
{
    return id_;
}

std::string WindowImpl::title() const
{
    return {};
}

std::string WindowImpl::className() const
{
    return {};
}

fs::path WindowImpl::ownerProcessPath() const
{
    return {};
}

uint64_t WindowImpl::ownerProcessId() const
{
    return 0;
}

Rect WindowImpl::boundingRect() const noexcept
{
    return {};
}

bool WindowImpl::capture(const ImageFormat format, std::vector<char>& content)
{
    std::ignore = format;
    std::ignore = content;
    return false;
}
