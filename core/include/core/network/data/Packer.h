#pragma once

#include "ISource.h"

namespace data {

class Packer
{
    ISource& source_;
    bool writeSize_ {true};

public:
    Packer(ISource& source);

    size_t get(std::string& buf, size_t maxSize = 64 * 1024);
};

} // namespace data
