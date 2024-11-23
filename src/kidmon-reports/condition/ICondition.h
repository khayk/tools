#pragma once

#include <memory>
#include <iosfwd>

struct Entry;

class ICondition
{
public:
    virtual ~ICondition() = default;

    virtual void write(std::ostream& os) const = 0;

    virtual bool met(const Entry& entry) const = 0;
};

using ConditionPtr = std::unique_ptr<ICondition>;