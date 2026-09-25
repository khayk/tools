#pragma once

#include <kidmon/repo/Filter.h>
#include <functional>
#include <span>

namespace km {

using UserCb = std::function<bool(const std::string&)>;
using EntryCb = std::function<bool(Entry&)>;

class IRepository
{
public:
    virtual ~IRepository() = default;

    virtual void add(const Entry& entry) = 0;

    /**
     * @brief Stores many entries at once, letting a repository amortize its
     * per-write cost. On failure some entries may already be stored, unless the
     * implementation documents otherwise. Defaults to calling add() in order.
     */
    virtual void addAll(std::span<const Entry> entries)
    {
        for (const auto& entry : entries)
        {
            add(entry);
        }
    }

    virtual void queryUsers(const UserCb& cb) const = 0;

    virtual void queryEntries(const Filter& filter, const EntryCb& cb) const = 0;
};

} // namespace km
