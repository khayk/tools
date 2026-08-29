#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <core/utils/Sys.h>
#include <core/utils/LogCapture.h>
#include <spdlog/spdlog.h>

#include <cerrno>

#ifdef _WIN32
    #include <Windows.h>
#endif

using namespace core::sys;
using testing::HasSubstr;
using testing::IsEmpty;
using testing::Not;

namespace {

// ---------------------------------------------------------------------------
// activeUserName
// ---------------------------------------------------------------------------

#if defined(_WIN32)

TEST(UtilsSysTests, ActiveUserNameNonEmpty)
{
    // On Windows a console session is always assigned, so the name is non-empty.
    EXPECT_FALSE(activeUserName().empty());
}

#elif defined(__APPLE__)

TEST(UtilsSysTests, ActiveUserNameNonEmptyOrThrows)
{
    // SCDynamicStoreCopyConsoleUser returns nullptr in headless / CI environments,
    // which causes the implementation to throw. Both outcomes are valid.
    try
    {
        EXPECT_FALSE(activeUserName().empty());
    }
    catch (const std::runtime_error&)
    {
        GTEST_SKIP() << "No console user active (headless/CI environment)";
    }
}

#else // Linux

TEST(UtilsSysTests, ActiveUserNameNonEmptyOrThrows)
{
    // Resolved from /etc/passwd or the USER/LOGNAME env vars. In an unnamed
    // environment (no passwd entry and no env) the implementation throws.
    try
    {
        EXPECT_FALSE(activeUserName().empty());
    }
    catch (const std::runtime_error&)
    {
        GTEST_SKIP() << "No resolvable user name in this environment";
    }
}

#endif

// ---------------------------------------------------------------------------
// currentProcessPath
// ---------------------------------------------------------------------------

TEST(UtilsSysTests, CurrentProcessPathStem)
{
    EXPECT_EQ(currentProcessPath().filename().stem().string(), "core-test");
}

TEST(UtilsSysTests, CurrentProcessPathIsAbsolute)
{
    EXPECT_TRUE(currentProcessPath().is_absolute());
}

TEST(UtilsSysTests, CurrentProcessPathExists)
{
    EXPECT_TRUE(fs::exists(currentProcessPath()));
}

// ---------------------------------------------------------------------------
// currentProcessId
// ---------------------------------------------------------------------------

TEST(UtilsSysTests, CurrentProcessIdIsPositive)
{
    EXPECT_GT(currentProcessId(), 0U);
}

TEST(UtilsSysTests, CurrentProcessIdIsStable)
{
    EXPECT_EQ(currentProcessId(), currentProcessId());
}

// ---------------------------------------------------------------------------
// errorDescription
// ---------------------------------------------------------------------------

TEST(UtilsSysTests, ErrorDescriptionDoesNotThrow)
{
    EXPECT_NO_THROW(errorDescription(0));
}

TEST(UtilsSysTests, ErrorDescriptionKnownCode)
{
    // ENOENT = "No such file or directory" on all supported platforms
    const auto desc = errorDescription(static_cast<uint64_t>(ENOENT));
    EXPECT_THAT(desc, Not(IsEmpty()));
    EXPECT_THAT(desc, HasSubstr("file"));
}

// ---------------------------------------------------------------------------
// constructErrorMsg
// ---------------------------------------------------------------------------

TEST(UtilsSysTests, ConstructErrorMsgZeroCode)
{
    // Code 0 means no error — only the message is returned
    EXPECT_EQ("something failed", constructErrorMsg("something failed", 0));
}

TEST(UtilsSysTests, ConstructErrorMsgNonZeroCode)
{
    const auto msg = constructErrorMsg("open failed", static_cast<uint64_t>(ENOENT));
    EXPECT_THAT(msg, HasSubstr("open failed"));
    EXPECT_THAT(msg, HasSubstr(std::to_string(ENOENT)));
    EXPECT_THAT(msg, HasSubstr("file"));
}

// ---------------------------------------------------------------------------
// constructLastErrorMsg
// ---------------------------------------------------------------------------

// Returns a seed error code the current platform uses for constructLastErrorMsg
// so the test can set it deterministically and verify the output.
struct LastErrorSetup
{
    uint64_t code;
    std::function<void()> set;
    std::function<void()> clear;
};

LastErrorSetup makeLastErrorSetup()
{
    // clang-format off
#ifdef _WIN32
    return {
        .code  = ERROR_FILE_NOT_FOUND,
        .set   = [] { SetLastError(ERROR_FILE_NOT_FOUND); },
        .clear = [] { SetLastError(0); },
    };
#else
    return {
        .code  = static_cast<uint64_t>(ENOENT),
        .set   = [] { errno = ENOENT; },
        .clear = [] { errno = 0; },
    };
#endif
    // clang-format on
}

TEST(UtilsSysTests, ConstructLastErrorMsgZeroCode)
{
    const auto setup = makeLastErrorSetup();
    setup.clear();
    EXPECT_EQ("op failed", constructLastErrorMsg("op failed"));
}

TEST(UtilsSysTests, ConstructLastErrorMsgWithError)
{
    const auto setup = makeLastErrorSetup();
    setup.set();
    const auto msg = constructLastErrorMsg("op failed");
    setup.clear();

    EXPECT_THAT(msg, HasSubstr("op failed"));
    EXPECT_THAT(msg, HasSubstr(std::to_string(setup.code)));
}

// ---------------------------------------------------------------------------
// logError
// ---------------------------------------------------------------------------

TEST(UtilsSysTests, LogErrorZeroCode)
{
    core::utl::LogCaptureMt cap;
    logError("op failed", 0);
    EXPECT_TRUE(cap.contains("op failed"));
}

TEST(UtilsSysTests, LogErrorNonZeroCode)
{
    core::utl::LogCaptureMt cap;
    logError("op failed", static_cast<uint64_t>(ENOENT));
    EXPECT_TRUE(cap.contains("op failed", spdlog::level::err));
    EXPECT_TRUE(cap.contains(std::to_string(ENOENT)));
    EXPECT_TRUE(cap.contains("file"));
}

// ---------------------------------------------------------------------------
// logLastError
// ---------------------------------------------------------------------------

TEST(UtilsSysTests, LogLastErrorZeroCodeLogsMessage)
{
    const auto setup = makeLastErrorSetup();
    setup.clear();
    core::utl::LogCaptureMt cap;
    logLastError("last error op");
    EXPECT_TRUE(cap.contains("last error op"));
}

TEST(UtilsSysTests, LogLastErrorNonZeroCodeLogsMessage)
{
    const auto setup = makeLastErrorSetup();
    core::utl::LogCaptureMt cap;
    setup.set();
    logLastError("last error op");
    setup.clear();
    EXPECT_TRUE(cap.contains("last error op", spdlog::level::err));
}

TEST(UtilsSysTests, LogLastErrorNonZeroCodeLogsErrorCode)
{
    const auto setup = makeLastErrorSetup();
    core::utl::LogCaptureMt cap;
    setup.set();
    logLastError("last error op");
    setup.clear();
    EXPECT_TRUE(cap.contains(std::to_string(setup.code)));
}

TEST(UtilsSysTests, LogLastErrorNonZeroCodeLogsDescription)
{
    const auto setup = makeLastErrorSetup();
    core::utl::LogCaptureMt cap;
    setup.set();
    logLastError("last error op");
    setup.clear();
    EXPECT_TRUE(cap.contains("file"));
}

// ---------------------------------------------------------------------------
// processMemoryUsage / currentProcessMemoryUsage
// ---------------------------------------------------------------------------

TEST(UtilsSysTests, CurrentProcessMemoryUsageNonZero)
{
    EXPECT_GT(currentProcessMemoryUsage(), 0);
}

TEST(UtilsSysTests, CurrentProcessMemoryUsageDoesNotThrow)
{
    EXPECT_NO_THROW(currentProcessMemoryUsage());
}

TEST(UtilsSysTests, ProcessMemoryUsageCurrentPid)
{
    EXPECT_NO_THROW(processMemoryUsage(currentProcessId()));
}

TEST(UtilsSysTests, ProcessMemoryUsageInvalidPid)
{
    // An unknown PID should not throw — implementation returns 0 gracefully
    EXPECT_NO_THROW(processMemoryUsage(0xFFFFFFFFU));
}

// ---------------------------------------------------------------------------
// currentProcessPeakMemoryUsage
// ---------------------------------------------------------------------------

TEST(UtilsSysTests, CurrentProcessPeakMemoryUsageNonZero)
{
    EXPECT_GT(currentProcessPeakMemoryUsage(), 0);
}

TEST(UtilsSysTests, CurrentProcessPeakMemoryUsageAtLeastCurrent)
{
    // The peak can never be smaller than the currently resident memory.
    EXPECT_GE(currentProcessPeakMemoryUsage(), currentProcessMemoryUsage());
}

} // namespace
