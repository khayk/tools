#pragma once

#include <memory>
#include <iosfwd>

namespace km {
struct Entry;
}

class ICondition
{
public:
    virtual ~ICondition() = default;

    virtual void write(std::ostream& os) const = 0;

    virtual bool met(const km::Entry& entry) const = 0;
};

using ConditionPtr = std::unique_ptr<ICondition>;