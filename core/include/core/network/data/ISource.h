#pragma once

#include <string>

namespace core::data {

class ISource
{
public:
    virtual ~ISource() = default;
    virtual size_t size() const noexcept = 0;
    virtual size_t get(std::string& buf, size_t maxSize) = 0;
};

} // namespace core::data
