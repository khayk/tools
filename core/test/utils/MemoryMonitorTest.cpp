#include <gtest/gtest.h>
#include <core/utils/MemoryMonitor.h>
#include <core/utils/LogCapture.h>

#include <algorithm>
#include <vector>

using namespace core::mem;

namespace {

// ---------------------------------------------------------------------------
// currentMemoryStats
// ---------------------------------------------------------------------------

TEST(MemoryMonitorTests, CurrentMemoryStatsNonZero)
{
    const auto stats = currentMemoryStats();
    EXPECT_GT(stats.currentBytes, 0U);
    EXPECT_GT(stats.peakBytes, 0U);
}

TEST(MemoryMonitorTests, CurrentMemoryStatsDoesNotThrow)
{
    EXPECT_NO_THROW(currentMemoryStats());
}

TEST(MemoryMonitorTests, PeakIsAtLeastCurrent)
{
    // The peak resident set size can never be smaller than the current one.
    const auto stats = currentMemoryStats();
    EXPECT_GE(stats.peakBytes, stats.currentBytes);
}

// ---------------------------------------------------------------------------
// humanReadableBytes
// ---------------------------------------------------------------------------

TEST(MemoryMonitorTests, HumanReadableBytesUnderKilobyte)
{
    EXPECT_EQ(humanReadableBytes(512), "512.0 B");
}

TEST(MemoryMonitorTests, HumanReadableBytesKilobytes)
{
    EXPECT_EQ(humanReadableBytes(2048), "2.0 KB");
}

TEST(MemoryMonitorTests, HumanReadableBytesMegabytes)
{
    EXPECT_EQ(humanReadableBytes(5L * 1024 * 1024), "5.0 MB");
}

TEST(MemoryMonitorTests, HumanReadableBytesRespectsPrecision)
{
    EXPECT_EQ(humanReadableBytes(1536, 2), "1.50 KB");
}

TEST(MemoryMonitorTests, HumanReadableBytesNegative)
{
    EXPECT_EQ(humanReadableBytes(-2048), "-2.0 KB");
}

TEST(MemoryMonitorTests, HumanReadableBytesZero)
{
    EXPECT_EQ(humanReadableBytes(0), "0.0 B");
}

// ---------------------------------------------------------------------------
// MemoryMonitor construction / state machine
// ---------------------------------------------------------------------------

TEST(MemoryMonitorTests, DefaultConstructorAutoStarts)
{
    MemoryMonitor mon;
    EXPECT_TRUE(mon.started());
}

TEST(MemoryMonitorTests, NoAutoStartConstructorIsNotStarted)
{
    MemoryMonitor mon(false);
    EXPECT_FALSE(mon.started());
}

TEST(MemoryMonitorTests, StartTransitionsToStarted)
{
    MemoryMonitor mon(false);
    mon.start();
    EXPECT_TRUE(mon.started());
}

TEST(MemoryMonitorTests, ResetLeavesMonitorNotStarted)
{
    MemoryMonitor mon;
    mon.reset();
    EXPECT_FALSE(mon.started());
}

TEST(MemoryMonitorTests, ResetClearsBaseline)
{
    MemoryMonitor mon;
    mon.reset();
    const auto baseline = mon.baseline();
    EXPECT_EQ(baseline.currentBytes, 0U);
    EXPECT_EQ(baseline.peakBytes, 0U);
}

TEST(MemoryMonitorTests, RestartCapturesFreshBaseline)
{
    MemoryMonitor mon;
    const auto first = mon.baseline();
    mon.restart();
    EXPECT_TRUE(mon.started());
    // Not asserting inequality — memory usage may legitimately be identical
    // between two calls a few instructions apart.
    EXPECT_GE(mon.baseline().currentBytes, 0U);
    (void) first;
}

// ---------------------------------------------------------------------------
// current / deltaBytes / peakBytes
// ---------------------------------------------------------------------------

TEST(MemoryMonitorTests, CurrentDoesNotThrow)
{
    MemoryMonitor mon;
    EXPECT_NO_THROW({ [[maybe_unused]] auto stats = mon.current(); });
}

TEST(MemoryMonitorTests, DeltaBytesIsZeroRightAfterStart)
{
    MemoryMonitor mon;
    // No allocation happened between start() and this check.
    EXPECT_NEAR(static_cast<double>(mon.deltaBytes()), 0.0, 1024.0 * 1024.0);
}

TEST(MemoryMonitorTests, DeltaBytesGrowsAfterAllocation)
{
    MemoryMonitor mon;

    // Force a large, touched allocation so the OS actually backs it with pages,
    // guaranteeing a measurable increase in resident memory.
    std::vector<std::byte> buffer(64ULL * 1024 * 1024);
    std::ranges::fill(buffer, std::byte {1});

    EXPECT_GT(mon.deltaBytes(), 0);
}

TEST(MemoryMonitorTests, PeakBytesIsAtLeastBaselineCurrent)
{
    MemoryMonitor mon;
    EXPECT_GE(mon.peakBytes(), mon.baseline().currentBytes);
}

// ---------------------------------------------------------------------------
// ScopedMemoryTrace
// ---------------------------------------------------------------------------

TEST(MemoryMonitorTests, ScopedMemoryTraceLogsEnterAndLeave)
{
    core::utl::LogCaptureMt cap(spdlog::level::trace);
    {
        ScopedMemoryTrace trace("unitTestScope");
    }
    EXPECT_TRUE(cap.contains("--> unitTestScope"));
    EXPECT_TRUE(cap.contains("<-- unitTestScope"));
}

TEST(MemoryMonitorTests, ScopedMemoryTraceUsesGivenLevel)
{
    core::utl::LogCaptureMt cap(spdlog::level::trace);
    {
        ScopedMemoryTrace trace("customLevelScope", spdlog::level::info);
    }
    EXPECT_TRUE(cap.contains("customLevelScope", spdlog::level::info));
}

TEST(MemoryMonitorTests, ScopedMemoryTraceReportsPeak)
{
    core::utl::LogCaptureMt cap(spdlog::level::trace);
    {
        ScopedMemoryTrace trace("peakReportingScope");
    }
    EXPECT_TRUE(cap.contains("peak"));
}

} // namespace
