#include <kidmon/repo/AsyncRepository.h>

#include <spdlog/spdlog.h>

namespace km {

namespace {

// Log a warning once the backlog crosses this size: it means the disk is not
// keeping up with the sampling rate. This is a visibility signal, not a hard
// cap -- silently dropping monitoring data is worse than a transient memory
// bump for a tool that samples only every few seconds.
constexpr std::size_t BACKLOG_WARN_THRESHOLD = 1024;

} // namespace

AsyncRepository::AsyncRepository(IRepository& inner)
    : inner_(inner)
    , worker_([this] {
        run();
    })
{
}

AsyncRepository::~AsyncRepository()
{
    {
        const std::lock_guard lock(mtx_);
        stop_ = true;
    }
    cv_.notify_one();

    if (worker_.joinable())
    {
        worker_.join();
    }
}

void AsyncRepository::add(const Entry& entry)
{
    std::size_t backlog = 0;
    {
        const std::lock_guard lock(mtx_);
        queue_.push_back(entry);
        backlog = queue_.size();
    }
    cv_.notify_one();

    // Warn only on the crossing edge so the log isn't spammed every add().
    if (backlog == BACKLOG_WARN_THRESHOLD)
    {
        spdlog::warn("Persistence backlog reached {} entries; disk may not be "
                     "keeping up with the sampling rate",
                     backlog);
    }
}

void AsyncRepository::run()
{
    // Reused across iterations so its capacity is retained between batches.
    std::deque<Entry> batch;
    std::unique_lock lock(mtx_);

    while (true)
    {
        cv_.wait(lock, [this] {
            return stop_ || !queue_.empty();
        });

        if (queue_.empty())
        {
            // Only reachable once stop_ is set and the backlog is fully drained.
            return;
        }

        // Take the whole backlog in a single O(1) swap, then release the lock
        // for the entire batch -- producers acquire the mutex once per batch
        // rather than once per entry, and never wait on disk. The single worker
        // still writes in FIFO order.
        batch.swap(queue_);
        lock.unlock();

        for (const auto& entry : batch)
        {
            try
            {
                inner_.add(entry);
            }
            catch (const std::exception& ex)
            {
                spdlog::error("Failed to persist entry: {}", ex.what());
            }
        }
        batch.clear();

        lock.lock();
    }
}

void AsyncRepository::queryUsers(const UserCb& cb) const
{
    inner_.queryUsers(cb);
}

void AsyncRepository::queryEntries(const Filter& filter, const EntryCb& cb) const
{
    inner_.queryEntries(filter, cb);
}

} // namespace km
