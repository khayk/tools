#include <gtest/gtest.h>
#include <core/utils/Tracer.h>
#include <core/utils/LogCapture.h>
#include <spdlog/spdlog.h>

namespace {

// ---------------------------------------------------------------------------
// ScopedTrace — enter / leave logging
// ---------------------------------------------------------------------------

TEST(TracerTests, LogsEnterOnConstruction)
{
    core::utl::LogCaptureMt cap;
    { ScopedTrace t("myFunc"); }
    EXPECT_TRUE(cap.contains("--> myFunc"));
}

TEST(TracerTests, LogsLeaveOnDestruction)
{
    core::utl::LogCaptureMt cap;
    { ScopedTrace t("myFunc"); }
    EXPECT_TRUE(cap.contains("<-- myFunc"));
}

TEST(TracerTests, BothEnterAndLeaveAreTraced)
{
    core::utl::LogCaptureMt cap;
    { ScopedTrace t("fn"); }
    EXPECT_EQ(cap.count(), 2U);
}

TEST(TracerTests, LogsAtTraceLevel)
{
    core::utl::LogCaptureMt cap;
    { ScopedTrace t("fn"); }
    const auto msgs = cap.messages();
    EXPECT_TRUE(std::ranges::all_of(msgs, [](const auto& e) {
        return e.level == spdlog::level::trace;
    }));
}

// ---------------------------------------------------------------------------
// extractFunction — namespace stripping via ScopedTrace
// ---------------------------------------------------------------------------

TEST(TracerTests, StripsLeadingNamespaceComponent)
{
    core::utl::LogCaptureMt cap;
    { ScopedTrace t("ns::myFunc"); }
    // "ns::" stripped — only "myFunc" should appear
    EXPECT_TRUE(cap.contains("myFunc"));
    EXPECT_FALSE(cap.contains("ns::myFunc"));
}

TEST(TracerTests, PlainNameWithoutNamespaceIsUnchanged)
{
    core::utl::LogCaptureMt cap;
    { ScopedTrace t("plainFunc"); }
    EXPECT_TRUE(cap.contains("plainFunc"));
}

// ---------------------------------------------------------------------------
// Custom enter / leave strings
// ---------------------------------------------------------------------------

TEST(TracerTests, CustomEnterString)
{
    core::utl::LogCaptureMt cap;
    { ScopedTrace t("fn", ">>", "<<"); }
    EXPECT_TRUE(cap.contains(">>fn"));
}

TEST(TracerTests, CustomLeaveString)
{
    core::utl::LogCaptureMt cap;
    { ScopedTrace t("fn", ">>", "<<"); }
    EXPECT_TRUE(cap.contains("<<fn"));
}

// ---------------------------------------------------------------------------
// isPrefix = false — suffix mode
// ---------------------------------------------------------------------------

TEST(TracerTests, SuffixModeAppendsEnterAfterMessage)
{
    core::utl::LogCaptureMt cap;
    { ScopedTrace t("fn", ">>", "<<", /*isPrefix=*/false); }
    EXPECT_TRUE(cap.contains("fn>>"));
}

TEST(TracerTests, SuffixModeAppendsLeaveAfterMessage)
{
    core::utl::LogCaptureMt cap;
    { ScopedTrace t("fn", ">>", "<<", /*isPrefix=*/false); }
    EXPECT_TRUE(cap.contains("fn<<"));
}

// ---------------------------------------------------------------------------
// Empty strings
// ---------------------------------------------------------------------------

TEST(TracerTests, EmptyEnterAndLeaveProducesOnlyMessage)
{
    core::utl::LogCaptureMt cap;
    { ScopedTrace t("alone", "", ""); }
    EXPECT_TRUE(cap.contains("alone"));
    EXPECT_EQ(cap.count(), 2U); // shouldTrace() is true because message_ is not empty
}

} // namespace
