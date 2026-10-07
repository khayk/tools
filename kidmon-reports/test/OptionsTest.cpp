#include <gtest/gtest.h>

#include "ArgsHelper.h"

using namespace km::reports;
using km::reports::test::parseArgs;

using Strings = std::vector<std::string>;

TEST(OptionsTest, MatchingIsCaseInsensitiveByDefault)
{
    EXPECT_FALSE(parseArgs({}).caseSensitive);
}

TEST(OptionsTest, CaseSensitiveFlagEnablesCaseSensitiveMatching)
{
    EXPECT_TRUE(parseArgs({"-c"}).caseSensitive);
    EXPECT_TRUE(parseArgs({"--case-sensitive"}).caseSensitive);
}

TEST(OptionsTest, CaseInsensitiveLowercasesIncludeAndExcludeNeedles)
{
    const auto conf = parseArgs({"-t",
                                 "YouTube",
                                 "-p",
                                 "Code",
                                 "--exclude-title",
                                 "Running Tests",
                                 "--exclude-process",
                                 "FireFox"});

    EXPECT_EQ(conf.titles, Strings {"youtube"});
    EXPECT_EQ(conf.processes, Strings {"code"});
    EXPECT_EQ(conf.excludeTitles, Strings {"running tests"});
    EXPECT_EQ(conf.excludeProcesses, Strings {"firefox"});
}

TEST(OptionsTest, CaseSensitiveKeepsNeedlesUnchanged)
{
    const auto conf = parseArgs({"-c",
                                 "-t",
                                 "YouTube",
                                 "-p",
                                 "Code",
                                 "--exclude-title",
                                 "Running Tests",
                                 "--exclude-process",
                                 "FireFox"});

    EXPECT_EQ(conf.titles, Strings {"YouTube"});
    EXPECT_EQ(conf.processes, Strings {"Code"});
    EXPECT_EQ(conf.excludeTitles, Strings {"Running Tests"});
    EXPECT_EQ(conf.excludeProcesses, Strings {"FireFox"});
}

TEST(OptionsTest, TopDefaultsToTen)
{
    EXPECT_EQ(parseArgs({}).topN, 10U);
    EXPECT_EQ(parseArgs({"-T", "3"}).topN, 3U);
}
