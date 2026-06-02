#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

namespace core {

/**
 * @brief Invoke @p fn(index) for every index in [0, count) across worker threads.
 *
 * Indices are handed out from a shared atomic counter, so threads that finish a
 * cheap item immediately pick up the next one. This keeps all cores busy even
 * when the per-item cost is wildly uneven (e.g. hashing a mix of tiny and
 * multi-gigabyte files).
 *
 * @p fn must be safe to call concurrently for distinct indices; it is never
 * invoked twice for the same index. Exceptions escaping @p fn would call
 * std::terminate, so @p fn is expected to handle its own errors.
 *
 * @param count   Number of items to process.
 * @param fn      Callable invoked as fn(std::size_t index).
 * @param threads Worker thread count; 0 (default) uses hardware concurrency.
 */
template <typename Fn>
void parallelFor(std::size_t count, Fn fn, unsigned threads = 0)
{
    if (count == 0)
    {
        return;
    }

    if (threads == 0)
    {
        threads = std::thread::hardware_concurrency();
    }
    threads = std::max(1U, threads);
    threads = static_cast<unsigned>(std::min<std::size_t>(threads, count));

    if (threads == 1)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            fn(i);
        }
        return;
    }

    std::atomic<std::size_t> next {0};

    auto worker = [&] {
        for (;;)
        {
            const std::size_t i = next.fetch_add(1, std::memory_order_relaxed);
            if (i >= count)
            {
                break;
            }
            fn(i);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(threads - 1);

    // Spawn threads-1 helpers and run one share on the calling thread.
    for (unsigned t = 1; t < threads; ++t)
    {
        workers.emplace_back(worker);
    }
    worker();

    for (auto& w : workers)
    {
        w.join();
    }
}

} // namespace core
