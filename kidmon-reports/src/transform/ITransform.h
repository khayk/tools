#pragma once

#include <memory>

struct Entry;

class ITransform
{
public:
    virtual ~ITransform() = default;

    virtual void apply(Entry& entry) = 0;
};

using TransformPtr = std::unique_ptr<ITransform>;
