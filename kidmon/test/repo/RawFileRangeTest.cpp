#include <gtest/gtest.h>

#include <repo/RawFileRange.h>

using namespace km::detail;

namespace {

// Representative raw file names. Within a year these sort chronologically via
// the zero-padded day-of-year prefix. Note that across years the ordering is
// meaningless: a December file ("raw-340-...") sorts *after* a January file
// ("raw-004-..."), which is exactly the inversion the boundary logic must not
// be confused by.
constexpr auto kJan05 = "raw-004-0105.dat"; // early-year day
constexpr auto kJun15 = "raw-165-0615.dat"; // mid-year day
constexpr auto kDec06 = "raw-339-1206.dat"; // late-year day

} // namespace

// ---------------------------------------------------------------------------
// yearInRange
// ---------------------------------------------------------------------------

TEST(RawFileRangeTest, YearWithinClosedRangeIsScanned)
{
    EXPECT_TRUE(yearInRange(2024, 2023, 2025));
    EXPECT_TRUE(yearInRange(2023, 2023, 2025)); // lower bound inclusive
    EXPECT_TRUE(yearInRange(2025, 2023, 2025)); // upper bound inclusive
}

TEST(RawFileRangeTest, YearOutsideClosedRangeIsSkipped)
{
    EXPECT_FALSE(yearInRange(2022, 2023, 2025));
    EXPECT_FALSE(yearInRange(2026, 2023, 2025));
}

TEST(RawFileRangeTest, ZeroBoundMeansUnbounded)
{
    // No lower bound: any year up to yearTo is in range.
    EXPECT_TRUE(yearInRange(1970, 0, 2025));
    EXPECT_FALSE(yearInRange(2026, 0, 2025));

    // No upper bound: any year from yearFrom on is in range.
    EXPECT_TRUE(yearInRange(9999, 2023, 0));
    EXPECT_FALSE(yearInRange(2022, 2023, 0));

    // Fully unbounded: every year is in range.
    EXPECT_TRUE(yearInRange(2024, 0, 0));
}

// ---------------------------------------------------------------------------
// rawFileInRange -- single-year query (yearFrom == yearTo == year)
// ---------------------------------------------------------------------------

TEST(RawFileRangeTest, SingleYearAcceptsFilesWithinBounds)
{
    // Range [Jan05, Dec06] in 2024.
    EXPECT_TRUE(rawFileInRange(kJun15, 2024, 2024, kJan05, 2024, kDec06));
    EXPECT_TRUE(rawFileInRange(kJan05, 2024, 2024, kJan05, 2024, kDec06)); // == from
    EXPECT_TRUE(rawFileInRange(kDec06, 2024, 2024, kJan05, 2024, kDec06)); // == to
}

TEST(RawFileRangeTest, SingleYearRejectsFilesOutsideBounds)
{
    // Range [Jun15, Dec06] in 2024 -- a January file is before `from`.
    EXPECT_FALSE(rawFileInRange(kJan05, 2024, 2024, kJun15, 2024, kDec06));

    // Range [Jan05, Jun15] in 2024 -- a December file is after `to`.
    EXPECT_FALSE(rawFileInRange(kDec06, 2024, 2024, kJan05, 2024, kJun15));
}

// ---------------------------------------------------------------------------
// rawFileInRange -- multi-year query, the year boundaries
// ---------------------------------------------------------------------------

TEST(RawFileRangeTest, StartYearAppliesOnlyTheLowerBound)
{
    // Range [Dec06 2023, Jan05 2024]. In the start year (2023) only `from`
    // constrains files; the end year's cutoff (kJan05) must NOT reject a
    // December file even though "raw-339..." > "raw-004..." lexicographically.
    EXPECT_TRUE(rawFileInRange(kDec06, 2023, 2023, kDec06, 2024, kJan05));

    // A file earlier than `from` within the start year is still rejected.
    EXPECT_FALSE(rawFileInRange(kJun15, 2023, 2023, kDec06, 2024, kJan05));
}

TEST(RawFileRangeTest, EndYearAppliesOnlyTheUpperBound)
{
    // Range [Dec06 2023, Jan05 2024]. In the end year (2024) only `to`
    // constrains files; the start year's cutoff (kDec06) must NOT reject a
    // January file even though "raw-004..." < "raw-339...".
    EXPECT_TRUE(rawFileInRange(kJan05, 2024, 2023, kDec06, 2024, kJan05));

    // A file later than `to` within the end year is still rejected.
    EXPECT_FALSE(rawFileInRange(kJun15, 2024, 2023, kDec06, 2024, kJan05));
}

TEST(RawFileRangeTest, IntermediateYearAcceptsEveryFile)
{
    // Range [Dec06 2022, Jan05 2024]. 2023 sits strictly between the bounds, so
    // every file in it is in range regardless of where it falls in the year.
    EXPECT_TRUE(rawFileInRange(kJan05, 2023, 2022, kDec06, 2024, kJan05));
    EXPECT_TRUE(rawFileInRange(kJun15, 2023, 2022, kDec06, 2024, kJan05));
    EXPECT_TRUE(rawFileInRange(kDec06, 2023, 2022, kDec06, 2024, kJan05));
}

// ---------------------------------------------------------------------------
// rawFileInRange -- unbounded sides
// ---------------------------------------------------------------------------

TEST(RawFileRangeTest, UnboundedLowerSideAcceptsEarlyFiles)
{
    // yearFrom == 0 / empty fnFrom: nothing is rejected by the lower bound.
    EXPECT_TRUE(rawFileInRange(kJan05, 2024, 0, "", 2024, kDec06));
}

TEST(RawFileRangeTest, UnboundedUpperSideAcceptsLateFiles)
{
    // yearTo == 0 / empty fnTo: nothing is rejected by the upper bound.
    EXPECT_TRUE(rawFileInRange(kDec06, 2024, 2024, kJan05, 0, ""));
}

TEST(RawFileRangeTest, EmptyBoundaryNameDoesNotReject)
{
    // An empty boundary name (buildRawFilename failed) must not act as a cutoff
    // even when the year matches; the per-entry timestamp check decides.
    EXPECT_TRUE(rawFileInRange(kJan05, 2024, 2024, "", 2024, ""));
}
