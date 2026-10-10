#include <gtest/gtest.h>

#include <QueryVisualizer.h>
#include <kidmon/data/Types.h>

#include <format>
#include <sstream>

using namespace km;
using namespace km::reports;
using namespace std::chrono_literals;

namespace {

Entry makeEntry(const std::string& processPath,
                const std::string& title,
                std::chrono::milliseconds duration)
{
    Entry entry;
    entry.processInfo.processPath = fs::path(processPath);
    entry.windowInfo.title = title;
    entry.timestamp.duration = duration;

    return entry;
}

} // namespace

TEST(QueryVisualizerTest, NothingProcessed)
{
    QueryVisualizer vis({});
    std::ostringstream oss;
    vis.display(oss);

    EXPECT_EQ(vis.numProcessed(), 0U);
    EXPECT_EQ(vis.numFiltered(), 0U);
    EXPECT_TRUE(oss.str().starts_with("Filtered: 0 out of 0\n"));
}

TEST(QueryVisualizerTest, ReportsFilteredOutOfProcessed)
{
    QueryVisualizer vis({});

    const auto code = makeEntry("/usr/bin/code", "Main.cpp", 1min);
    const auto firefox = makeEntry("/usr/bin/firefox", "YouTube", 2min);
    const auto chess = makeEntry("/usr/bin/chess", "Game", 3min);

    // All entries are processed, only some pass the condition
    for (const auto& entry : {code, firefox, chess})
    {
        vis.update(entry);
    }
    vis.add(code);
    vis.add(firefox);

    std::ostringstream oss;
    vis.display(oss);
    const auto out = oss.str();

    EXPECT_EQ(vis.numProcessed(), 3U);
    EXPECT_EQ(vis.numFiltered(), 2U);
    EXPECT_TRUE(out.starts_with("Filtered: 2 out of 3\n")) << out;
    EXPECT_TRUE(out.contains("code")) << out;
    EXPECT_TRUE(out.contains("firefox")) << out;
    EXPECT_FALSE(out.contains("chess")) << out;
}

namespace {

size_t countRows(const std::string& out, std::string_view field)
{
    size_t count = 0;
    std::istringstream iss(out);

    for (std::string line; std::getline(iss, line);)
    {
        count += line.contains(field) ? 1 : 0;
    }

    return count;
}

std::string displayWithTop(const TopN& top)
{
    QueryVisualizer vis({.top_ = top});

    // 3 processes, 3 titles each, longer durations for earlier ones
    for (const auto* proc : {"/usr/bin/aaa", "/usr/bin/bbb", "/usr/bin/ccc"})
    {
        for (int i = 0; i < 3; ++i)
        {
            vis.add(makeEntry(proc, std::format("{}-title-{}", proc, i), 1min));
        }
    }
    vis.add(makeEntry("/usr/bin/aaa", "/usr/bin/aaa-title-0", 10min));
    vis.add(makeEntry("/usr/bin/bbb", "/usr/bin/bbb-title-0", 5min));

    std::ostringstream oss;
    vis.display(oss);

    return oss.str();
}

} // namespace

TEST(QueryVisualizerTest, SingleTopAppliesToEveryLevel)
{
    const auto out = displayWithTop({2});

    EXPECT_EQ(countRows(out, "proc_name:"), 2U) << out;
    EXPECT_EQ(countRows(out, "title:"), 4U) << out;
    EXPECT_TRUE(out.contains("aaa")) << out;
    EXPECT_FALSE(out.contains("ccc")) << out;
}

TEST(QueryVisualizerTest, LastTopAppliesToDeeperLevels)
{
    const auto out = displayWithTop({3, 1});

    EXPECT_EQ(countRows(out, "proc_name:"), 3U) << out;
    EXPECT_EQ(countRows(out, "proc_path:"), 3U) << out;
    EXPECT_EQ(countRows(out, "title:"), 3U) << out;
    EXPECT_TRUE(out.contains("aaa-title-0")) << out;
    EXPECT_FALSE(out.contains("aaa-title-1")) << out;
}

TEST(QueryVisualizerTest, EmptyTopShowsEverything)
{
    const auto out = displayWithTop({});

    EXPECT_EQ(countRows(out, "proc_name:"), 3U) << out;
    EXPECT_EQ(countRows(out, "title:"), 9U) << out;
}

TEST(QueryVisualizerTest, EmptyTitleIsShown)
{
    QueryVisualizer vis({.top_ = {1}});
    vis.add(makeEntry("/usr/bin/chrome", "", 10min));
    vis.add(makeEntry("/usr/bin/chrome", "News", 1min));

    std::ostringstream oss;
    vis.display(oss);
    const auto out = oss.str();

    EXPECT_TRUE(out.contains("title: <empty>, duration: 10m")) << out;
    EXPECT_FALSE(out.contains("News")) << out;
}

TEST(QueryVisualizerTest, ZeroTopHidesLevelAndBelow)
{
    const auto procsOnly = displayWithTop({2, 0});

    EXPECT_EQ(countRows(procsOnly, "proc_name:"), 2U) << procsOnly;
    EXPECT_EQ(countRows(procsOnly, "proc_path:"), 0U) << procsOnly;
    EXPECT_EQ(countRows(procsOnly, "title:"), 0U) << procsOnly;

    const auto totalOnly = displayWithTop({0});

    EXPECT_EQ(countRows(totalOnly, "proc_name:"), 0U) << totalOnly;
    EXPECT_TRUE(totalOnly.contains("total duration: 24m")) << totalOnly;
}
