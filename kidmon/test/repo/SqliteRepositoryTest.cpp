#include <gtest/gtest.h>

#include <kidmon/repo/SqliteRepository.h>
#include <core/utils/File.h>

#include <chrono>
#include <unordered_set>
#include <vector>

using namespace km;
using namespace std::chrono_literals;
using core::file::TempDir;

namespace {

// Storage truncates capture time to milliseconds, so samples must be built
// already truncated to compare equal after a round trip.
TimePoint truncateToMillis(TimePoint tp)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    return TimePoint(duration_cast<milliseconds>(tp.time_since_epoch()));
}

Entry sampleEntry(const std::string& username,
                  TimePoint capture = truncateToMillis(SystemClock::now()),
                  std::chrono::milliseconds duration = 1s)
{
    Entry entry;
    entry.username = username;
    entry.processInfo.processPath = "proc.exe";
    entry.processInfo.sha256 = "deadbeef";
    entry.windowInfo.title = "title";
    entry.windowInfo.placement = Rect(Point(-10, 7), Dimensions(1920, 1080));
    entry.windowInfo.image.name = "img.jpg";
    entry.windowInfo.image.bytes = std::string("\x00\x01 not-really-a-jpeg\xff", 21);
    entry.windowInfo.image.encoded = false;
    entry.timestamp.capture = capture;
    entry.timestamp.duration = duration;
    return entry;
}

fs::path tempDbFile(const TempDir& dir)
{
    return dir.path() / "kidmon.db";
}

} // namespace

TEST(SqliteRepositoryTest, DbFileCreatedOnConstruction)
{
    TempDir dir("kdmn-tst");
    const auto dbFile = tempDbFile(dir);

    SqliteRepository repo(dbFile);

    EXPECT_EQ(dbFile, repo.dbFile());
    EXPECT_TRUE(fs::exists(dbFile));
}

TEST(SqliteRepositoryTest, AddAndQuerySingleEntryRoundTrips)
{
    TempDir dir("kdmn-tst");
    SqliteRepository repo(tempDbFile(dir));

    const Entry entry = sampleEntry("john");
    EXPECT_NO_THROW(repo.add(entry));

    std::vector<Entry> found;
    const Filter filter(entry.username);
    repo.queryEntries(filter, [&found](Entry& e) {
        found.push_back(e);
        return true;
    });

    ASSERT_EQ(found.size(), 1U);
    EXPECT_EQ(found.front(), entry);
}

TEST(SqliteRepositoryTest, QueryUsersEnumeratesDistinctUsersAndCanStopEarly)
{
    TempDir dir("kdmn-tst");
    SqliteRepository repo(tempDbFile(dir));

    repo.add(sampleEntry("alice"));
    repo.add(sampleEntry("alice")); // same user twice -- must not duplicate the row
    repo.add(sampleEntry("bob"));

    std::unordered_set<std::string> users;
    repo.queryUsers([&users](const std::string& username) {
        users.insert(username);
        return true;
    });
    EXPECT_EQ(users, (std::unordered_set<std::string> {"alice", "bob"}));

    int visited = 0;
    repo.queryUsers([&visited](const std::string&) {
        ++visited;
        return false; // stop after the first
    });
    EXPECT_EQ(visited, 1);
}

TEST(SqliteRepositoryTest, QueryEntriesFiltersByTimeRangeAndOrdersChronologically)
{
    TempDir dir("kdmn-tst");
    SqliteRepository repo(tempDbFile(dir));

    const auto base = truncateToMillis(SystemClock::now());
    std::vector<Entry> entries;
    entries.reserve(5);
    for (int i = 0; i < 5; ++i)
    {
        entries.push_back(sampleEntry("john", base + i * 1min));
    }

    // Insert out of order to prove the query does the ordering.
    repo.add(entries[3]);
    repo.add(entries[0]);
    repo.add(entries[4]);
    repo.add(entries[1]);
    repo.add(entries[2]);

    const Filter filter("john",
                        entries[1].timestamp.capture,
                        entries[3].timestamp.capture);

    std::vector<TimePoint> seen;
    repo.queryEntries(filter, [&seen](Entry& e) {
        seen.push_back(e.timestamp.capture);
        return true;
    });

    const std::vector<TimePoint> expected {entries[1].timestamp.capture,
                                           entries[2].timestamp.capture,
                                           entries[3].timestamp.capture};
    EXPECT_EQ(seen, expected);
}

TEST(SqliteRepositoryTest, QueryEntriesStopsWhenCallbackReturnsFalse)
{
    TempDir dir("kdmn-tst");
    SqliteRepository repo(tempDbFile(dir));

    const auto base = truncateToMillis(SystemClock::now());
    for (int i = 0; i < 3; ++i)
    {
        repo.add(sampleEntry("john", base + i * 1s));
    }

    int visited = 0;
    repo.queryEntries(Filter("john"), [&visited](Entry&) {
        ++visited;
        return false;
    });
    EXPECT_EQ(visited, 1);
}

TEST(SqliteRepositoryTest, QueryEntriesForUnknownUserYieldsNothing)
{
    TempDir dir("kdmn-tst");
    SqliteRepository repo(tempDbFile(dir));

    repo.add(sampleEntry("john"));

    int visited = 0;
    repo.queryEntries(Filter("someone-else"), [&visited](Entry&) {
        ++visited;
        return true;
    });
    EXPECT_EQ(visited, 0);
}

TEST(SqliteRepositoryTest, EntryWithNoSnapshotRoundTrips)
{
    TempDir dir("kdmn-tst");
    SqliteRepository repo(tempDbFile(dir));

    Entry entry = sampleEntry("john");
    entry.windowInfo.image = Image {};
    repo.add(entry);

    std::vector<Entry> found;
    repo.queryEntries(Filter("john"), [&found](Entry& e) {
        found.push_back(e);
        return true;
    });

    ASSERT_EQ(found.size(), 1U);
    EXPECT_EQ(found.front(), entry);
}
