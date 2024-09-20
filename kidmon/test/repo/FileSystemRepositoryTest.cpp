#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <kidmon/repo/FileSystemRepository.h>
#include <core/utils/File.h>

#include <fmt/format.h>

using namespace std::chrono_literals;
using ::testing::Return;
using ::testing::_;

namespace {

Timestamp sampleTimestamp(TimePoint capture = SystemClock::now(),
                          std::chrono::milliseconds dur = 1s)
{
    return {capture, dur};
}

Rect sampleRect(const Point lt = {0, 7}, const Dimensions dims = {42, 7})
{
    return {lt, dims};
}

ProcessInfo sampleProcInfo(const fs::path& proc = "proc.exe",
                           const std::string& sha256 = "sha256")
{
    return {proc, sha256};
}

Image sampleImage(const std::string& name = "img",
                  const std::string& bytes = "",
                  bool encoded = false)
{
    return {name, bytes, encoded};
}

WindowInfo sampleWndInfo(const std::string& title = "title",
                         const Rect& plcmnt = sampleRect(),
                         const Image& img = sampleImage())
{
    return {
        plcmnt,
        img,
        title
    };
}

Entry sampleEntry(const std::string& username = "john",
                      const ProcessInfo& pi = sampleProcInfo(),
                      const WindowInfo& wi = sampleWndInfo(),
                      const Timestamp ts = sampleTimestamp())
{
    return {username, pi, wi, ts};
}

class MockRepo : public FileSystemRepository
{
public: 
    MockRepo(fs::path reportsDir)
        : FileSystemRepository(reportsDir)
    {
    }
    
    // Overrides
    MOCK_METHOD(void, add, (const Entry& entry), (override));
    MOCK_METHOD(void, queryUsers, (const UserCb& cb), (const, override));
    MOCK_METHOD(void,
                queryEntries,
                (const Filter& filter, const EntryCb& cb),
                (const, override));
};

} // namespace

using namespace file;

TEST(FileSystemRepositoryTest, ReportsDirNotCreated)
{
    TempDir reportsDir("kdmn-tst", TempDir::CreateMode::Manual);
    FileSystemRepository repo(reportsDir.path());

    EXPECT_EQ(reportsDir.path(), repo.reportsDir());
    
    // Directory will be created when we add something
    EXPECT_FALSE(fs::exists(reportsDir.path()));
}

TEST(FileSystemRepositoryTest, ReportsDirCreated)
{
    file::TempDir reportsDir("kdmn-tst", TempDir::CreateMode::Manual);
    FileSystemRepository repo(reportsDir.path());

    const Entry entry = sampleEntry();    
    EXPECT_NO_THROW(repo.add(entry));
    EXPECT_TRUE(fs::exists(reportsDir.path()));
}

void bindActions(MockRepo& repo)
{
    ON_CALL(repo, add(_)).WillByDefault([&repo](const Entry& entry) {
        repo.FileSystemRepository::add(entry);
    });

    ON_CALL(repo, queryUsers(_)).WillByDefault([&repo](const auto& cb) {
        repo.FileSystemRepository::queryUsers(cb);
    });

    ON_CALL(repo, queryEntries(_, _))
        .WillByDefault([&repo](const auto& filter, const auto& cb) {
            repo.FileSystemRepository::queryEntries(filter, cb);
        });
}

TEST(FileSystemRepositoryTest, AddEntryOneUser)
{
    file::TempDir reportsDir("kdmn-tst");
    MockRepo repo(reportsDir.path());

    EXPECT_CALL(repo, add(_)).Times(1);
    EXPECT_CALL(repo, queryUsers(_)).Times(1);
    bindActions(repo);

    const Entry entry = sampleEntry();
    EXPECT_NO_THROW(repo.add(entry));

    repo.queryUsers([&entry](const std::string& usename) {
        EXPECT_EQ(entry.username, usename);
        return true;
    });
}

TEST(FileSystemRepositoryTest, AddEntriesOneUser)
{
    file::TempDir reportsDir("kdmn-tst");
    MockRepo repo(reportsDir.path());

    EXPECT_CALL(repo, add(_)).Times(3);
    EXPECT_CALL(repo, queryUsers(_)).Times(1);
    bindActions(repo);

    const Entry entry = sampleEntry();
    EXPECT_NO_THROW(repo.add(entry));
    EXPECT_NO_THROW(repo.add(entry));
    EXPECT_NO_THROW(repo.add(entry));
    
    repo.queryUsers([&entry](const std::string& usename) {
        EXPECT_EQ(entry.username, usename);
        return true;
    });
}

TEST(FileSystemRepositoryTest, QueryEntriesMultipleUsers)
{
    file::TempDir reportsDir("kdmn-tst");
    MockRepo repo(reportsDir.path());
    
    constexpr int numUsers = 3;
    constexpr int numEntriesPerUser = 4;

    EXPECT_CALL(repo, add(_)).Times(numUsers * numEntriesPerUser);
    EXPECT_CALL(repo, queryUsers(_)).Times(1);
    EXPECT_CALL(repo, queryEntries(_, _)).Times(numUsers);
    bindActions(repo);

    std::vector<Entry> entries;

    for (int i = 0; i < numUsers; ++i)
    {
        const auto name = fmt::format("name-{}", i);
        
        for (int j = 1; j <= numEntriesPerUser; ++j)
        {
            auto ts = sampleTimestamp(SystemClock::now() + (i * numEntriesPerUser + j) * 1s, j * 1s);
            const auto x = std::chrono::duration_cast<std::chrono::milliseconds>(ts.capture.time_since_epoch());
            ts.capture = TimePoint {x};

            entries.push_back(
                sampleEntry(
                    name, 
                    sampleProcInfo(), 
                    sampleWndInfo(), 
                    ts
                )
            );     
        }
    }

    // Deliberately add them in a wrong order
    for (const auto& e: entries)
    {
        EXPECT_NO_THROW(repo.add(e));
    }

    int usersEnumrated = 0;
    repo.queryUsers([&entries, &usersEnumrated, i = 0](const std::string& usename) mutable {
        EXPECT_EQ(entries[i].username, usename);
        i += numEntriesPerUser;
        ++usersEnumrated;
        return true;
    });
    EXPECT_EQ(usersEnumrated, numUsers);

    int entriesEnumarated = 0;
    for (int i = 0; i < numUsers; ++i)
    {
        Filter filter(entries[numEntriesPerUser * i].username);
        repo.queryEntries(filter,
                          [&entries, j = numEntriesPerUser * i, &entriesEnumarated](
                              const Entry& entry) mutable {
            EXPECT_EQ(entries[j], entry);
            ++j;
            ++entriesEnumarated;
            return true;
        });  
    }

    EXPECT_EQ(numEntriesPerUser * numUsers, entriesEnumarated);
}