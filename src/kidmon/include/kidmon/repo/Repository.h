#pragma once

#include <kidmon/repo/Filter.h>
#include <functional>

using UserCb = std::function<bool(const std::string&)>;
using EntryCb = std::function<bool(Entry&)>;

class IRepository
{
public:
    virtual ~IRepository() = default;

    virtual void add(const Entry& entry) = 0;

    virtual void queryUsers(const UserCb& cb) const = 0;

    virtual void queryEntries(const Filter& filter, const EntryCb& cb) const = 0;
};
