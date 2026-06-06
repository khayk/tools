#include <gtest/gtest.h>

#include <kidmon/repo/AsyncRepository.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "core/utils/StopWatch.h"

using namespace km;
using namespace std::chrono_literals;

namespace {

// Minimal in-memory IRepository that records the order in which entries are
// persisted. add() can be made artificially slow to exercise the off-thread
// behaviour of AsyncRepository.
class FakeRepository : public IRepository
{
public:
    void add(const Entry& entry) override
    {
        if (addDelay_.count() > 0)
        {
            std::this_thread::sleep_for(addDelay_);
        }

        {
            const std::lock_guard lock(mtx_);
            usernames_.push_back(entry.username);
        }
        cv_.notify_all();
    }

    void queryUsers(const UserCb&) const override {}

    void queryEntries(const Filter&, const EntryCb&) const override {}

    void setAddDelay(std::chrono::milliseconds delay)
    {
        addDelay_ = delay;
    }

    // Waits until at least @p count entries have been persisted, or the timeout
    // elapses. Returns the recorded usernames in persistence order.
    std::vector<std::string> waitFor(std::size_t count,
                                     std::chrono::milliseconds timeout)
    {
        std::unique_lock lock(mtx_);
        cv_.wait_for(lock, timeout, [&] {
            return usernames_.size() >= count;
        });
        return usernames_;
    }

    std::size_t size() const
    {
        const std::lock_guard lock(mtx_);
        return usernames_.size();
    }

private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::vector<std::string> usernames_;
    std::chrono::milliseconds addDelay_ {0};
};

Entry makeEntry(std::string username)
{
    Entry entry;
    entry.username = std::move(username);
    return entry;
}

} // namespace

TEST(AsyncRepositoryTest, ForwardsEntriesInOrder)
{
    FakeRepository fake;
    {
        AsyncRepository repo(fake);
        repo.add(makeEntry("a"));
        repo.add(makeEntry("b"));
        repo.add(makeEntry("c"));

        const auto seen = fake.waitFor(3, 2s);
        EXPECT_EQ(seen, (std::vector<std::string> {"a", "b", "c"}));
    }
}

TEST(AsyncRepositoryTest, AddDoesNotBlockOnSlowWrite)
{
    FakeRepository fake;
    fake.setAddDelay(200ms);

    AsyncRepository repo(fake);

    // The slow write happens on the worker thread, so add() must return well
    // before the underlying write completes.
    StopWatch sw;
    repo.add(makeEntry("slow"));

    EXPECT_LT(sw.elapsed(), 100ms);

    // The write still lands eventually.
    const auto seen = fake.waitFor(1, 2s);
    EXPECT_EQ(seen, (std::vector<std::string> {"slow"}));
}

TEST(AsyncRepositoryTest, FlushesPendingEntriesOnDestruction)
{
    FakeRepository fake;
    fake.setAddDelay(20ms);
    {
        AsyncRepository repo(fake);
        for (int i = 0; i < 10; ++i)
        {
            repo.add(makeEntry(std::to_string(i)));
        }
        // Leaving the scope must drain the backlog before the worker joins.
    }

    EXPECT_EQ(fake.size(), 10U);
}
