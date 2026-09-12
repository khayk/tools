#include <gtest/gtest.h>

#include <kidmon/repo/RepositoryMigrator.h>
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
    destination.failNextAdds(1); // only the second add() fails

    const auto stats = migrate(source, destination);

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
    const auto stats =
        migrate(source, destination, [](const MigrationStats& progress) {
            return progress.entriesCopied < 1; // stop right after the first entry
        });

    EXPECT_EQ(stats.entriesCopied, 1U);
    EXPECT_LT(destination.countFor("alice") + destination.countFor("bob"), 3U);
}
