#include <gtest/gtest.h>

#include <QueryVisualizer.h>
#include <kidmon/data/Types.h>

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
