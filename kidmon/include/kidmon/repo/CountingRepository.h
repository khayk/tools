#pragma once

#include "Repository.h"

#include <atomic>
#include <cstddef>

namespace km {

/**
 * @brief IRepository decorator that counts successful and failed writes.
 *
 * Feeds the server's periodic status line. Counters are atomic, so writes may
 * come from a worker thread while take() is called from another.
 */
class CountingRepository : public IRepository
{
public:
    struct Counts
    {
        std::size_t stored {};
        std::size_t failed {};
    };

    explicit CountingRepository(IRepository& inner);

    void add(const Entry& entry) override;
    void addAll(std::span<const Entry> entries) override;
    void queryUsers(const UserCb& cb) const override;
    void queryEntries(const Filter& filter, const EntryCb& cb) const override;

    // Returns the counts accumulated since the previous call and resets them.
    Counts take() noexcept;

private:
    IRepository& inner_;
    std::atomic<std::size_t> stored_ {0};
    std::atomic<std::size_t> failed_ {0};
};

} // namespace km
