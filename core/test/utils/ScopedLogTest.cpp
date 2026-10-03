#include <gtest/gtest.h>
#include <core/utils/ScopedLog.h>
#include <core/utils/LogCapture.h>
#include <spdlog/spdlog.h>

namespace {

// ---------------------------------------------------------------------------
// ScopedLog — enter / leave logging
// ---------------------------------------------------------------------------

TEST(ScopedLogTests, LogsEnterOnConstruction)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("myFunc");
    }
    EXPECT_TRUE(cap.contains("--> myFunc"));
}

TEST(ScopedLogTests, LogsLeaveOnDestruction)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("myFunc");
    }
    EXPECT_TRUE(cap.contains("<-- myFunc"));
}

TEST(ScopedLogTests, BothEnterAndLeaveAreLogged)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("fn");
    }
    EXPECT_EQ(cap.count(), 2U);
}

// ---------------------------------------------------------------------------
// Log level
// ---------------------------------------------------------------------------

TEST(ScopedLogTests, LogsAtTraceLevelByDefault)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("fn");
    }
    const auto msgs = cap.messages();
    EXPECT_TRUE(std::ranges::all_of(msgs, [](const auto& e) {
        return e.level == spdlog::level::trace;
    }));
}

TEST(ScopedLogTests, LogsAtGivenLevel)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("fn", spdlog::level::info);
    }
    const auto msgs = cap.messages();
    ASSERT_EQ(msgs.size(), 2U);
    EXPECT_TRUE(std::ranges::all_of(msgs, [](const auto& e) {
        return e.level == spdlog::level::info;
    }));
}

TEST(ScopedLogTests, NothingLoggedBelowLoggerLevel)
{
    core::utl::LogCaptureMt cap;
    const auto prevLevel = spdlog::get_level();
    spdlog::set_level(spdlog::level::info);
    {
        ScopedLog t("fn", spdlog::level::debug);
    }
    spdlog::set_level(prevLevel);
    EXPECT_EQ(cap.count(), 0U);
}

// ---------------------------------------------------------------------------
// extractFunction — namespace stripping via ScopedLog
// ---------------------------------------------------------------------------

TEST(ScopedLogTests, StripsLeadingNamespaceComponent)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("ns::myFunc");
    }
    // "ns::" stripped — only "myFunc" should appear
    EXPECT_TRUE(cap.contains("myFunc"));
    EXPECT_FALSE(cap.contains("ns::myFunc"));
}

TEST(ScopedLogTests, PlainNameWithoutNamespaceIsUnchanged)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("plainFunc");
    }
    EXPECT_TRUE(cap.contains("plainFunc"));
}

// ---------------------------------------------------------------------------
// Custom enter / leave strings
// ---------------------------------------------------------------------------

TEST(ScopedLogTests, CustomEnterString)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("fn", spdlog::level::trace, ">>", "<<");
    }
    EXPECT_TRUE(cap.contains(">>fn"));
}

TEST(ScopedLogTests, CustomLeaveString)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("fn", spdlog::level::trace, ">>", "<<");
    }
    EXPECT_TRUE(cap.contains("<<fn"));
}

// ---------------------------------------------------------------------------
// isPrefix = false — suffix mode
// ---------------------------------------------------------------------------

TEST(ScopedLogTests, SuffixModeAppendsEnterAfterMessage)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("fn", spdlog::level::trace, ">>", "<<", /*isPrefix=*/false);
    }
    EXPECT_TRUE(cap.contains("fn>>"));
}

TEST(ScopedLogTests, SuffixModeAppendsLeaveAfterMessage)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("fn", spdlog::level::trace, ">>", "<<", /*isPrefix=*/false);
    }
    EXPECT_TRUE(cap.contains("fn<<"));
}

// ---------------------------------------------------------------------------
// Empty strings
// ---------------------------------------------------------------------------

TEST(ScopedLogTests, EmptyEnterAndLeaveProducesOnlyMessage)
{
    core::utl::LogCaptureMt cap;
    {
        ScopedLog t("alone", spdlog::level::trace, "", "");
    }
    EXPECT_TRUE(cap.contains("alone"));
    EXPECT_EQ(cap.count(), 2U); // shouldLog() is true because message_ is not empty
}

} // namespace
