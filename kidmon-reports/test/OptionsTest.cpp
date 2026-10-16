#include <gtest/gtest.h>

#include "ArgsHelper.h"

#include <array>

using namespace km::reports;
using km::reports::test::optionConflict;
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
    using TopN = std::vector<uint32_t>;

    EXPECT_EQ(parseArgs({}).top, TopN {10});
    EXPECT_EQ(parseArgs({"-T", "3"}).top, TopN {3});
    EXPECT_EQ(parseArgs({"--top", "10,3"}).top, (TopN {10, 3}));
}

TEST(OptionsTest, ShortFlagsForHelpAndHours)
{
    auto opts = makeOptions();
    const std::array argv = {"kidmon-reports", "-h", "-H", "3"};
    const auto result = opts.parse(static_cast<int>(argv.size()), argv.data());

    EXPECT_TRUE(result.contains("help"));
    EXPECT_EQ(result["hours"].as<uint32_t>(), 3U);
}

TEST(OptionsTest, NoConflictForValidCombinations)
{
    EXPECT_FALSE(optionConflict({}));
    EXPECT_FALSE(optionConflict({"-u", "alice", "-d", "1", "-T", "5"}));
    EXPECT_FALSE(optionConflict({"-l", "--source", "a.db"}));
    EXPECT_FALSE(optionConflict({"--migrate-to", "a.db", "--source", "/r"}));
}

TEST(OptionsTest, ConflictingCommandsAndOptions)
{
    EXPECT_EQ(optionConflict({"-l", "--migrate-to", "a.db"}),
              "--list and --migrate-to can't be used together");
    EXPECT_EQ(optionConflict({"--migrate-to", "a.db", "-u", "alice"}),
              "--user can't be used with --migrate-to");
    EXPECT_EQ(optionConflict({"-l", "-d", "1"}), "--days can't be used with --list");
    EXPECT_EQ(optionConflict({"-l", "-T", "5"}), "--top can't be used with --list");
}
