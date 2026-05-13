#pragma once

#include <memory>

namespace km {
struct Entry;
}

class ITransform
{
public:
    virtual ~ITransform() = default;

    virtual void apply(km::Entry& entry) = 0;
};

using TransformPtr = std::unique_ptr<ITransform>;
