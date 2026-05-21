#include <gtest/gtest.h>
#include <core/utils/StopWatch.h>

#include <thread>

namespace {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(StopWatchTests, DefaultConstructorAutoStarts)
{
    StopWatch sw;
    EXPECT_FALSE(sw.stopped());
}

TEST(StopWatchTests, NoAutoStartConstructorIsStopped)
{
    StopWatch sw(false);
    EXPECT_TRUE(sw.stopped());
}

// ---------------------------------------------------------------------------
// start / stop state machine
// ---------------------------------------------------------------------------

TEST(StopWatchTests, StartTransitionsToRunning)
{
    StopWatch sw(false);
    sw.start();
    EXPECT_FALSE(sw.stopped());
}

TEST(StopWatchTests, StopTransitionsToStopped)
{
    StopWatch sw;
    sw.stop();
    EXPECT_TRUE(sw.stopped());
}

TEST(StopWatchTests, StartIsIdempotentWhenAlreadyRunning)
{
    StopWatch sw;
    sw.start(); // second call while running — should be a no-op
    EXPECT_FALSE(sw.stopped());
}

TEST(StopWatchTests, StopIsIdempotentWhenAlreadyStopped)
{
    StopWatch sw;
    sw.stop();
    sw.stop(); // second call while stopped — should be a no-op
    EXPECT_TRUE(sw.stopped());
}

// ---------------------------------------------------------------------------
// reset
// ---------------------------------------------------------------------------

TEST(StopWatchTests, ResetLeavesWatchStopped)
{
    StopWatch sw;
    sw.reset();
    EXPECT_TRUE(sw.stopped());
}

TEST(StopWatchTests, ResetClearsElapsed)
{
    StopWatch sw;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sw.stop();
    sw.reset();
    EXPECT_EQ(sw.elapsedMs(), 0);
}

// ---------------------------------------------------------------------------
// restart
// ---------------------------------------------------------------------------

TEST(StopWatchTests, RestartLeavesWatchRunning)
{
    StopWatch sw;
    sw.stop();
    sw.restart();
    EXPECT_FALSE(sw.stopped());
}

TEST(StopWatchTests, RestartResetsElapsed)
{
    StopWatch sw;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sw.restart();
    EXPECT_LT(sw.elapsedMs(), 5); // restarted just now — elapsed must be tiny
}

// ---------------------------------------------------------------------------
// elapsed / elapsedMs
// ---------------------------------------------------------------------------

TEST(StopWatchTests, ElapsedMsMatchesElapsedCount)
{
    StopWatch sw;
    sw.stop();
    EXPECT_EQ(sw.elapsedMs(), sw.elapsed().count());
}

TEST(StopWatchTests, ElapsedGrowsWhileRunning)
{
    StopWatch sw;
    const auto first = sw.elapsedMs();
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    const auto second = sw.elapsedMs();
    EXPECT_GE(second, first);
}

TEST(StopWatchTests, ElapsedIsStableAfterStop)
{
    StopWatch sw;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sw.stop();
    const auto snap1 = sw.elapsedMs();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const auto snap2 = sw.elapsedMs();
    EXPECT_EQ(snap1, snap2);
}

TEST(StopWatchTests, ElapsedIsNonNegativeWhenNotStarted)
{
    StopWatch sw(false);
    EXPECT_GE(sw.elapsedMs(), 0);
}

} // namespace
