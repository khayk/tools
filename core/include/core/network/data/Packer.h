#pragma once

#include <core/network/data/ISource.h>

namespace core::data {

class Packer
{
    ISource& source_;
    bool writeSize_ {true};

public:
    Packer(ISource& source);

    size_t get(std::string& buf, size_t maxSize = 64UL * 1024UL);
};

} // namespace core::data
