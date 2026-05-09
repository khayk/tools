#include "Window.h"
#include <format>

const std::string& WindowImpl::id() const
{
    return id_;
}

std::string WindowImpl::title() const
{
    throw std::logic_error(std::format("Not implemented: {}", __func__));
    return {};
}

std::string WindowImpl::className() const
{
    throw std::logic_error(std::format("Not implemented: {}", __func__));
    return {};
}

fs::path WindowImpl::ownerProcessPath() const
{
    throw std::logic_error(std::format("Not implemented: {}", __func__));
    return {};
}

uint64_t WindowImpl::ownerProcessId() const
{
    throw std::logic_error(std::format("Not implemented: {}", __func__));
    return 0;
}

Rect WindowImpl::boundingRect() const noexcept
{
    return {};
}

bool WindowImpl::capture(const ImageFormat format, std::vector<char>& content)
{
    throw std::logic_error(std::format("Not implemented: {}", __func__));
    std::ignore = format;
    std::ignore = content;
    return false;
}
