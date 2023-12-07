#pragma once

#include <string>

namespace data {

class ISource
{
public:
    virtual size_t size() const noexcept = 0;
    virtual size_t get(std::string& buf, size_t maxSize) = 0;
};

} // namespace data
