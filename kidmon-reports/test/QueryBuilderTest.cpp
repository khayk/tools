#include <gtest/gtest.h>

#include "ArgsHelper.h"

#include <QueryBuilder.h>
#include <condition/ICondition.h>
#include <kidmon/data/Types.h>

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

// Mirrors how the app evaluates an entry: transform first, then the condition
bool matches(const ReportsConfig& conf, Entry entry)
{
    const auto condition = buildCondition(conf);
    const auto transform = buildTransform(conf);

    transform->apply(entry);

    return condition->met(entry);
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
