#include <gtest/gtest.h>

#include "ArgsHelper.h"

#include <QueryBuilder.h>
#include <condition/ICondition.h>
#include <kidmon/common/Utils.h>
#include <kidmon/data/Types.h>
#include <kidmon/repo/FileSystemRepository.h>
#include <core/utils/File.h>

#include <ctime>
#include <stdexcept>

using namespace km;
using namespace km::reports;
using km::reports::test::parseArgs;

namespace {

Entry makeEntry(const std::string& processPath, const std::string& title)
{
    Entry entry;
    entry.processInfo.processPath = fs::path(processPath);
    entry.windowInfo.title = title;

    return entry;
}

bool matches(const ReportsConfig& conf, const Entry& entry)
{
    return buildCondition(conf)->met(entry);
}

} // namespace

TEST(QueryBuilderTest, IncludeTitleIgnoresCaseByDefault)
{
    const auto conf = parseArgs({"-t", "YouTube"});

    EXPECT_TRUE(matches(conf, makeEntry("/usr/bin/firefox", "youtube - Cats")));
    EXPECT_TRUE(matches(conf, makeEntry("/usr/bin/firefox", "YOUTUBE - Cats")));
    EXPECT_FALSE(matches(conf, makeEntry("/usr/bin/firefox", "Wikipedia")));
}

TEST(QueryBuilderTest, IncludeProcessIgnoresCaseByDefault)
{
    const auto conf = parseArgs({"-p", "firefox"});

    EXPECT_TRUE(matches(conf, makeEntry("/Applications/Firefox.app/Firefox", "")));
    EXPECT_FALSE(matches(conf, makeEntry("/usr/bin/code", "")));
}

TEST(QueryBuilderTest, ExcludeTitleIgnoresCaseByDefault)
{
    const auto conf = parseArgs({"--exclude-title", "Running Tests"});

    EXPECT_FALSE(matches(conf, makeEntry("/usr/bin/code", "running tests")));
    EXPECT_FALSE(matches(conf, makeEntry("/usr/bin/code", "RUNNING TESTS: core")));
    EXPECT_TRUE(matches(conf, makeEntry("/usr/bin/code", "Editing Main.cpp")));
}

TEST(QueryBuilderTest, ExcludeProcessIgnoresCaseByDefault)
{
    const auto conf = parseArgs({"--exclude-process", "FireFox"});

    EXPECT_FALSE(matches(conf, makeEntry("/Applications/Firefox.app/firefox", "")));
    EXPECT_TRUE(matches(conf, makeEntry("/usr/bin/code", "")));
}

TEST(QueryBuilderTest, CaseSensitiveExcludeRespectsCase)
{
    const auto conf = parseArgs({"-c", "--exclude-title", "Running"});

    EXPECT_FALSE(matches(conf, makeEntry("/usr/bin/code", "Running tests")));
    EXPECT_TRUE(matches(conf, makeEntry("/usr/bin/code", "running tests")));
}

TEST(QueryBuilderTest, CaseSensitiveIncludeRespectsCase)
{
    const auto conf = parseArgs({"-c", "-t", "YouTube"});

    EXPECT_TRUE(matches(conf, makeEntry("/usr/bin/firefox", "YouTube - Cats")));
    EXPECT_FALSE(matches(conf, makeEntry("/usr/bin/firefox", "youtube - Cats")));
}

namespace {

TimePoint localMidnight(int year, int month, int day)
{
    std::tm tm = {};
    tm.tm_mday = day;
    tm.tm_mon = month - 1;
    tm.tm_year = year - 1900;
    tm.tm_isdst = -1;

    return SystemClock::from_time_t(std::mktime(&tm));
}

} // namespace

TEST(QueryBuilderTest, RangeIncludesItsEndDate)
{
    const auto filter = buildFilter(parseArgs({"-r", "20240601,20240630"}));

    EXPECT_EQ(filter.from(), localMidnight(2024, 6, 1));
    EXPECT_EQ(filter.to(), localMidnight(2024, 7, 1));
}

TEST(QueryBuilderTest, RangeOfOneDayCoversThatDay)
{
    const auto filter = buildFilter(parseArgs({"-r", "20241231,20241231"}));

    EXPECT_EQ(filter.from(), localMidnight(2024, 12, 31));
    EXPECT_EQ(filter.to(), localMidnight(2025, 1, 1));
}

TEST(QueryBuilderTest, RangeWithoutEndLastsUntilNow)
{
    const auto before = SystemClock::now();
    const auto filter = buildFilter(parseArgs({"-r", "20240601"}));

    EXPECT_EQ(filter.from(), localMidnight(2024, 6, 1));
    EXPECT_GE(filter.to(), before);
}

TEST(QueryBuilderTest, InvalidRangeThrows)
{
    for (const auto* range : {"20241345",
                              "20240230",
                              "2024",
                              "19691231",
                              "20240601,20240001",
                              "20240601,20240531",
                              "20240101,20240201,20240301"})
    {
        EXPECT_THROW(buildFilter(parseArgs({"-r", range})), std::invalid_argument)
            << range;
    }
}

TEST(QueryBuilderTest, NoTimeOptionReportsToday)
{
    const auto now = std::time(nullptr);
    const auto today = km::utl::timet2tm(now);
    const auto filter = buildFilter(parseArgs({}));

    EXPECT_EQ(filter.from(),
              localMidnight(today.tm_year + 1900, today.tm_mon + 1, today.tm_mday));
    EXPECT_GE(filter.to(), SystemClock::from_time_t(now));
}

TEST(QueryBuilderTest, RelativeTimeGoesBackFromNow)
{
    const auto filter = buildFilter(parseArgs({"-h", "2", "-m", "30"}));

    EXPECT_EQ(filter.to() - filter.from(), std::chrono::minutes(150));
}

TEST(QueryBuilderTest, ValidateUser)
{
    core::file::TempDir reportsDir("kdmn-rep-tst");
    fs::create_directories(reportsDir.path() / "alice");
    const FileSystemRepository repo(reportsDir.path());

    EXPECT_NO_THROW(validateUser(repo, "alice"));
    EXPECT_THROW(validateUser(repo, ""), std::invalid_argument);
    EXPECT_THROW(validateUser(repo, "bob"), std::invalid_argument);
}

TEST(QueryBuilderTest, CaseInsensitiveMatchingHandlesNonAscii)
{
    const auto conf = parseArgs({"-p", "ÉDITEUR", "-t", "ПРИВЕТ"});

    EXPECT_TRUE(
        matches(conf, makeEntry("/Applications/Éditeur.app/éditeur", "привет")));
    EXPECT_FALSE(
        matches(conf, makeEntry("/Applications/Éditeur.app/éditeur", "пока")));
}
