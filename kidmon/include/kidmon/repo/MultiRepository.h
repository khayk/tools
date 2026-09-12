#pragma once

#include "Repository.h"
#include <vector>
#include <functional>

namespace km {

/**
 * @brief IRepository decorator that fans add() out to several repositories.
 */
class MultiRepository : public IRepository
{
public:
    // `repos` must not be empty; repos[0] is the primary used for queries.
    explicit MultiRepository(std::vector<std::reference_wrapper<IRepository>> repos);

    // Convenience for the common two-repository migration case.
    MultiRepository(IRepository& primary, IRepository& secondary);

    void add(const Entry& entry) override;
    void queryUsers(const UserCb& cb) const override;
    void queryEntries(const Filter& filter, const EntryCb& cb) const override;

private:
    std::vector<std::reference_wrapper<IRepository>> repos_;
};

} // namespace km
