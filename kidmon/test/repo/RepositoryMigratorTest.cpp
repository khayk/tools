#include <gtest/gtest.h>

#include <kidmon/repo/RepositoryMigrator.h>
#include <kidmon/repo/FileSystemRepository.h>
#include <core/utils/File.h>
#include "support/FakeRepository.h"

#include <chrono>

using namespace km;
using namespace std::chrono_literals;
using km::test::FakeRepository;

namespace {

Entry makeEntry(std::string username, TimePoint capture)
{
    Entry entry;
    entry.username = std::move(username);
    entry.timestamp.capture = capture;
    entry.timestamp.duration = 1s;
    return entry;
}

} // namespace

TEST(RepositoryMigratorTest, EmptySourceMigratesNothing)
{
    FakeRepository source;
    FakeRepository destination;

    const auto stats = migrate(source, destination);

    EXPECT_EQ(stats.usersSeen, 0U);
    EXPECT_EQ(stats.entriesCopied, 0U);
    EXPECT_EQ(stats.entriesFailed, 0U);
}

TEST(RepositoryMigratorTest, CopiesEveryUserAndEntry)
{
    FakeRepository source;
    const auto now = SystemClock::now();
    source.add(makeEntry("alice", now));
    source.add(makeEntry("alice", now + 1s));
    source.add(makeEntry("bob", now));

    FakeRepository destination;
    const auto stats = migrate(source, destination);

    EXPECT_EQ(stats.usersSeen, 2U);
    EXPECT_EQ(stats.entriesCopied, 3U);
    EXPECT_EQ(stats.entriesFailed, 0U);
    EXPECT_EQ(destination.countFor("alice"), 2U);
    EXPECT_EQ(destination.countFor("bob"), 1U);
}

TEST(RepositoryMigratorTest, DestinationFailureIsCountedButDoesNotAbortTheRun)
{
    FakeRepository source;
    const auto now = SystemClock::now();
    source.add(makeEntry("alice", now));
    source.add(makeEntry("alice", now + 1s));
    source.add(makeEntry("alice", now + 2s));

    FakeRepository destination;
    // The first entry fails as a chunk of one and again on its retry.
    destination.failNextAdds(2);

    // chunkSize 1 so each entry is its own write and only one fails
    const auto stats = migrate(source, destination, {}, 1);

    EXPECT_EQ(stats.entriesCopied, 2U);
    EXPECT_EQ(stats.entriesFailed, 1U);
    EXPECT_EQ(destination.countFor("alice"), 2U);
}

TEST(RepositoryMigratorTest, ProgressCallbackCanCancelTheRunEarly)
{
    FakeRepository source;
    const auto now = SystemClock::now();
    source.add(makeEntry("alice", now));
    source.add(makeEntry("alice", now + 1s));
    source.add(makeEntry("bob", now));

    FakeRepository destination;
    const auto stats = migrate(
        source,
        destination,
        [](const MigrationStats& progress) {
            return progress.entriesCopied < 1; // stop right after the first chunk
        },
        1);

    EXPECT_EQ(stats.entriesCopied, 1U);
    EXPECT_LT(destination.countFor("alice") + destination.countFor("bob"), 3U);
}

TEST(RepositoryMigratorTest, WritesEntriesInChunks)
{
    FakeRepository source;
    const auto now = SystemClock::now();
    for (int i = 0; i < 5; ++i)
    {
        source.add(makeEntry("alice", now + std::chrono::seconds(i)));
    }

    FakeRepository destination;
    const auto stats = migrate(source, destination, {}, 2);

    EXPECT_EQ(stats.entriesCopied, 5U);
    EXPECT_EQ(destination.countFor("alice"), 5U);
    EXPECT_EQ(destination.addAllCalls(), 3U); // 2 + 2 + 1
}

TEST(RepositoryMigratorTest, FailedChunkIsRetriedEntryByEntry)
{
    FakeRepository source;
    const auto now = SystemClock::now();
    source.add(makeEntry("alice", now));
    source.add(makeEntry("alice", now + 1s));
    source.add(makeEntry("alice", now + 2s));

    FakeRepository destination;
    // The chunk's first add() fails, aborting addAll(); the retry then stores
    // all three.
    destination.failNextAdds(1);

    const auto stats = migrate(source, destination, {}, 3);

    EXPECT_EQ(stats.entriesCopied, 3U);
    EXPECT_EQ(stats.entriesFailed, 0U);
    EXPECT_EQ(destination.countFor("alice"), 3U);
}

// The file repository reuses one Entry per raw file, so the migrator must copy
// rather than move out of it.
TEST(RepositoryMigratorTest, KeepsUsernameWhenSourceReusesEntries)
{
    core::file::TempDir reportsDir("kdmn-tst");
    FileSystemRepository source(reportsDir.path());
    const auto now = std::chrono::floor<std::chrono::milliseconds>(SystemClock::now());
    for (int i = 0; i < 3; ++i)
    {
        source.add(makeEntry("alice", now + std::chrono::seconds(i)));
    }

    FakeRepository destination;
    const auto stats = migrate(source, destination);

    EXPECT_EQ(stats.entriesCopied, 3U);
    EXPECT_EQ(destination.countFor("alice"), 3U);
    EXPECT_EQ(destination.countFor(""), 0U);
}
