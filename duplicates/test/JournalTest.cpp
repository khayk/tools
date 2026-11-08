#include <gtest/gtest.h>

#include <duplicates/Journal.h>
#include <core/utils/File.h>
#include <core/utils/Log.h>
#include <core/utils/LogCapture.h>

#include <array>
#include <string>

namespace tools::dups {

using namespace core;
using utl::LogCaptureSt;
using utl::MuteLogger;

namespace {

fs::path u8path(std::u8string_view s)
{
    return {s};
}

} // namespace

TEST(JournalTest, FormatQuotesBothPaths)
{
    const JournalEntry entry {"/src/a.jpg", "/cache/run/h/a.jpg"};
    EXPECT_EQ(formatEntry(entry), R"("/src/a.jpg"|"/cache/run/h/a.jpg")");
}

TEST(JournalTest, RoundTripsUnusualPaths)
{
    const std::array entries {
        JournalEntry {"/plain/file.txt", "/backup/file.txt"},
        JournalEntry {"/with spaces/my photo.jpg", "/b/my photo.jpg"},
        JournalEntry {"/pipe|in/name|x.jpg", "/b/name|x.jpg"},
        JournalEntry {R"(/quote"in/na"me.jpg)", R"(/b/na"me.jpg)"},
        JournalEntry {R"(C:\Users\me\Photos\a.jpg)", R"(D:\cache\run\h\a.jpg)"},
        JournalEntry {R"(/trailing\)", R"(/b/back\slash)"},
        JournalEntry {u8path(u8"/Фото/Երևան/写真.jpg"), u8path(u8"/b/写真.jpg")},
    };

    for (const auto& entry : entries)
    {
        const auto line = formatEntry(entry);
        const auto parsed = parseEntry(line);

        EXPECT_EQ(parsed, entry) << line;
    }
}

TEST(JournalTest, ParseToleratesTrailingWhitespaceAndCr)
{
    const JournalEntry expected {"/a", "/b"};

    EXPECT_EQ(parseEntry("\"/a\"|\"/b\"\r"), expected);
    EXPECT_EQ(parseEntry("\"/a\"|\"/b\"  \t"), expected);
}

TEST(JournalTest, ParseRejectsMalformedLines)
{
    for (const auto* line : {
             "",
             "/a|/b",         // unquoted
             R"("/a"|/b)",    // second field unquoted
             R"("/a""/b")",   // missing separator
             R"("/a","/b")",  // wrong separator
             R"("/a"|)",      // missing second field
             R"("/a"|"/b)",   // unterminated quote
             R"("/a"|"/b"x)", // trailing garbage
             R"(""|"/b")",    // empty original
             R"("/a"|"")",    // empty backup
             R"( "/a"|"/b")", // leading whitespace
         })
    {
        EXPECT_FALSE(parseEntry(line).has_value()) << line;
    }
}

TEST(JournalTest, WriterCreatesFileLazily)
{
    file::TempDir dir("dups");
    const auto path = dir.path() / std::string(JOURNAL_FILE_NAME);

    JournalWriter writer(path);
    EXPECT_EQ(writer.path(), path);
    EXPECT_FALSE(fs::exists(path));

    writer.append({"/a", "/b"});
    EXPECT_TRUE(fs::exists(path));
}

TEST(JournalTest, WriterFlushesEachEntry)
{
    file::TempDir dir("dups");
    const auto path = dir.path() / "journal.log";

    // The writer stays open: every entry must already be readable from disk.
    JournalWriter writer(path);
    writer.append({"/a", "/b"});
    EXPECT_EQ(readJournal(path).size(), 1U);

    writer.append({"/c", "/d"});
    EXPECT_EQ(readJournal(path).size(), 2U);
}

TEST(JournalTest, WriteThenReadPreservesOrder)
{
    file::TempDir dir("dups");
    const auto path = dir.path() / "journal.log";
    const std::vector<JournalEntry> entries {
        {"/x/3.jpg", "/b/3.jpg"},
        {"/x/1 one.jpg", "/b/1 one.jpg"},
        {u8path(u8"/x/Ժ.jpg"), u8path(u8"/b/Ժ.jpg")},
    };

    {
        JournalWriter writer(path);
        for (const auto& e : entries)
        {
            writer.append(e);
        }
    }

    EXPECT_EQ(readJournal(path), entries);
}

TEST(JournalTest, WriterAppendsToExistingJournal)
{
    file::TempDir dir("dups");
    const auto path = dir.path() / "journal.log";

    JournalWriter(path).append({"/a", "/b"});
    JournalWriter(path).append({"/c", "/d"});

    const std::vector<JournalEntry> expected {{"/a", "/b"}, {"/c", "/d"}};
    EXPECT_EQ(readJournal(path), expected);
}

TEST(JournalTest, WriterThrowsWhenFileCannotBeOpened)
{
    MuteLogger mute;
    file::TempDir dir("dups");
    JournalWriter writer(dir.path() / "missing-dir" / "journal.log");

    EXPECT_THROW(writer.append({"/a", "/b"}), std::system_error);
}

TEST(JournalTest, ReadSkipsEmptyAndMalformedLines)
{
    LogCaptureSt log;
    file::TempDir dir("dups");
    const auto path = dir.path() / "journal.log";

    file::write(path,
                "\"/a\"|\"/b\"\n"
                "\n"
                "garbage\n"
                "  \r\n"
                "\"/c\"|\"/d\"\r\n"
                "\"/e\"|\"/f\""); // no trailing newline

    const std::vector<JournalEntry> expected {
        {"/a", "/b"},
        {"/c", "/d"},
        {"/e", "/f"},
    };
    EXPECT_EQ(readJournal(path), expected);
    EXPECT_TRUE(log.contains("Skipping malformed journal line 3"));
}

TEST(JournalTest, ReadEmptyJournal)
{
    file::TempDir dir("dups");
    const auto path = dir.path() / "journal.log";
    file::write(path, "");

    EXPECT_TRUE(readJournal(path).empty());
}

} // namespace tools::dups
