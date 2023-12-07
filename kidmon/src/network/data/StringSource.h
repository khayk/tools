#pragma once

#include "ISource.h"

namespace data {

class StringSource : public ISource
{
public:
    StringSource(std::string_view src);

    size_t size() const noexcept override;

    size_t get(std::string& buf, size_t maxSize) override;

private:
    std::string src_;
    std::string_view sv_;
};

} // namespace data
