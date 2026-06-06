#pragma once

#include <kidmon/repo/Repository.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace km {

/**
 * @brief IRepository decorator that performs writes on a dedicated background
 *        thread so blocking disk I/O never runs on the caller's thread.
 *
 * The kidmon server processes agent messages on a single asio event-loop
 * thread. Persisting an entry involves blocking filesystem writes (screenshot
 * bytes plus a raw-record append); doing that inline stalls all networking and
 * the health-check timer. AsyncRepository moves add() off that thread: it copies
 * the entry into an internal queue and returns immediately, while one worker
 * thread drains the queue and forwards each entry to the wrapped repository.
 *
 * Writes are serialized in FIFO order by the single worker, preserving the
 * append order the underlying repository expects. Entries still queued at
 * destruction are flushed before the worker joins, so nothing is lost on a
 * clean shutdown.
 *
 * queryUsers() / queryEntries() forward synchronously to the wrapped repository
 * (they are not used on the server hot path). A query does not observe writes
 * that are still queued.
 */
class AsyncRepository : public IRepository
{
public:
    explicit AsyncRepository(IRepository& inner);
    ~AsyncRepository() override;

    AsyncRepository(const AsyncRepository&) = delete;
    AsyncRepository& operator=(const AsyncRepository&) = delete;
    AsyncRepository(AsyncRepository&&) = delete;
    AsyncRepository& operator=(AsyncRepository&&) = delete;

    void add(const Entry& entry) override;
    void queryUsers(const UserCb& cb) const override;
    void queryEntries(const Filter& filter, const EntryCb& cb) const override;

private:
    void run();

    IRepository& inner_;

    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::deque<Entry> queue_;
    bool stop_ {false};

    std::jthread worker_;
};

} // namespace km
