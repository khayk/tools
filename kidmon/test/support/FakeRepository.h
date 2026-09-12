#pragma once

#include <kidmon/repo/Repository.h>

#include <cstddef>
#include <map>
#include <stdexcept>
#include <vector>

// Minimal in-memory IRepository shared by MultiRepository/RepositoryMigrator tests.
namespace km::test {

class FakeRepository : public IRepository
{
public:
    void add(const Entry& entry) override
    {
        ++addCalls_;
        if (failNextAdds_ > 0)
        {
            --failNextAdds_;
            throw std::runtime_error("FakeRepository: simulated add() failure");
        }
        byUser_[entry.username].push_back(entry);
    }

    void queryUsers(const UserCb& cb) const override
    {
        for (const auto& kv : byUser_)
        {
            if (!cb(kv.first))
            {
                return;
            }
        }
    }

    void queryEntries(const Filter& filter, const EntryCb& cb) const override
    {
        const auto it = byUser_.find(filter.username());
        if (it == byUser_.end())
        {
            return;
        }

        for (const auto& stored : it->second)
        {
            if (stored.timestamp.capture < filter.from() ||
                stored.timestamp.capture > filter.to())
            {
                continue;
            }

            Entry copy = stored;
            if (!cb(copy))
            {
                return;
            }
        }
    }

    // Entries stored for `username` (0 if never seen).
    [[nodiscard]] std::size_t countFor(const std::string& username) const
    {
        const auto it = byUser_.find(username);
        return it == byUser_.end() ? 0 : it->second.size();
    }

    // Every add() attempt, including ones that fail (unlike countFor()).
    [[nodiscard]] std::size_t addCalls() const noexcept
    {
        return addCalls_;
    }

    // Makes the next N add() calls throw instead of storing.
    void failNextAdds(std::size_t count) noexcept
    {
        failNextAdds_ = count;
    }

private:
    std::map<std::string, std::vector<Entry>> byUser_;
    std::size_t addCalls_ {0};
    std::size_t failNextAdds_ {0};
};

} // namespace km::test
