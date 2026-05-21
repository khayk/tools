#include <gtest/gtest.h>
#include <core/utils/SingleInstanceChecker.h>
#include <core/utils/LogCapture.h>

using core::SingleInstanceChecker;
using core::utl::LogCaptureSt;

namespace {

// Unique name unlikely to be held by any real process during tests.
constexpr std::wstring_view kTestApp = L"CoreTest_SingleInstanceChecker";

// ---------------------------------------------------------------------------
// processAlreadyRunning — common
// ---------------------------------------------------------------------------

TEST(SingleInstanceCheckerTests, FirstInstanceNotRunning)
{
    SingleInstanceChecker sic(kTestApp);
    EXPECT_FALSE(sic.processAlreadyRunning());
}

// ---------------------------------------------------------------------------
// processAlreadyRunning — Windows and macOS (real implementation)
// ---------------------------------------------------------------------------

#if defined(_WIN32) || defined(__APPLE__)

TEST(SingleInstanceCheckerTests, SecondInstanceDetectedAsRunning)
{
    SingleInstanceChecker first(kTestApp);
    ASSERT_FALSE(first.processAlreadyRunning());

    SingleInstanceChecker second(kTestApp);
    EXPECT_TRUE(second.processAlreadyRunning());
}

TEST(SingleInstanceCheckerTests, AfterDestructionNewInstanceIsNotRunning)
{
    {
        SingleInstanceChecker first(kTestApp);
        ASSERT_FALSE(first.processAlreadyRunning());
    }
    SingleInstanceChecker fresh(kTestApp);
    EXPECT_FALSE(fresh.processAlreadyRunning());
}

TEST(SingleInstanceCheckerTests, DifferentNamesDoNotConflict)
{
    SingleInstanceChecker a(L"CoreTest_SingleInstanceA");
    SingleInstanceChecker b(L"CoreTest_SingleInstanceB");
    EXPECT_FALSE(a.processAlreadyRunning());
    EXPECT_FALSE(b.processAlreadyRunning());
}

// ---------------------------------------------------------------------------
// processAlreadyRunning — Linux (stub)
// ---------------------------------------------------------------------------

#else

TEST(SingleInstanceCheckerTests, AlwaysReportsNotRunning)
{
    SingleInstanceChecker a(kTestApp);
    SingleInstanceChecker b(kTestApp);
    EXPECT_FALSE(a.processAlreadyRunning());
    EXPECT_FALSE(b.processAlreadyRunning());
}

TEST(SingleInstanceCheckerTests, ConstructorLogsNotImplemented)
{
    LogCaptureSt cap;
    SingleInstanceChecker sic(kTestApp);
    EXPECT_TRUE(cap.contains("not implemented"));
}

#endif

// ---------------------------------------------------------------------------
// report
// ---------------------------------------------------------------------------

TEST(SingleInstanceCheckerTests, ReportLogsAppName)
{
    SingleInstanceChecker sic(kTestApp);
    LogCaptureSt cap;
    sic.report();
    EXPECT_TRUE(cap.contains("CoreTest_SingleInstanceChecker"));
}

} // namespace
