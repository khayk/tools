#pragma once

#include <kidmon/repo/Filter.h>
#include <functional>

class IRepository
{
public:
    using UserCb = std::function<void(const std::string&)>;
    using EntryCb = std::function<void(const Entry&)>;

    virtual ~IRepository() = default;

    virtual void add(const Entry& entry) = 0;

    virtual void queryUsers(UserCb cb) const = 0;

    virtual void queryEntries(const Filter& filter, EntryCb cb) const = 0;
};
