#include <gtest/gtest.h>

#include <core/utils/Parallel.h>

#include <algorithm>
#include <atomic>
#include <numeric>
#include <vector>

using namespace core;

namespace {

TEST(UtilsParallelTests, EmptyRangeDoesNothing)
{
    std::atomic<int> calls {0};
    parallelFor(0, [&](std::size_t) {
        ++calls;
    });
    EXPECT_EQ(calls.load(), 0);
}

TEST(UtilsParallelTests, VisitsEveryIndexExactlyOnce)
{
    constexpr std::size_t count = 10'000;
    std::vector<int> hits(count, 0);

    parallelFor(count, [&](std::size_t i) {
        // Distinct indices => distinct slots, no synchronisation required.
        ++hits[i];
    });

    for (std::size_t i = 0; i < count; ++i)
    {
        EXPECT_EQ(hits[i], 1) << "index " << i;
    }
}

TEST(UtilsParallelTests, ConcurrentWorkAggregatesCorrectly)
{
    constexpr std::size_t count = 100'000;
    std::vector<std::size_t> values(count);
    std::ranges::iota(values, std::size_t {0});

    std::atomic<std::size_t> sum {0};
    parallelFor(count, [&](std::size_t i) {
        sum.fetch_add(values[i], std::memory_order_relaxed);
    });

    EXPECT_EQ(sum.load(), count * (count - 1) / 2);
}

TEST(UtilsParallelTests, SingleThreadRequestStillCoversRange)
{
    constexpr std::size_t count = 256;
    std::vector<int> hits(count, 0);

    parallelFor(
        count,
        [&](std::size_t i) {
            ++hits[i];
        },
        1);

    EXPECT_TRUE(std::ranges::all_of(hits, [](int h) {
        return h == 1;
    }));
}

} // namespace
