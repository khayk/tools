#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DeletionStrategy.h>
#include <duplicates/Progress.h>
#include <core/utils/File.h>
#include <core/utils/Log.h>
#include <core/utils/LogInterceptor.h>

#include <filesystem>

using testing::MockFunction;
using testing::Return;

namespace tools::dups {

using utl::SilenceLogger;
using utl::TestStLogInterceptor;

namespace {

class MockDelete : public IDeletionStrategy
{
public:
    MOCK_METHOD(void, remove, (const fs::path& file), (const, override));
};


class MockDeletionStrategy : public IDeletionStrategy
{
public:
    MOCK_METHOD(void, remove, (const fs::path&), (const, override));
};


class MockDuplicateGroups : public IDuplicateGroups
{
public:
    MOCK_METHOD((size_t), numGroups, (), (const, noexcept, override));
    MOCK_METHOD((void), enumGroups, (const DupGroupCallback&), (const, override));
};

void emulateDupGroups(const std::vector<PathsVec>& groupVec,
                      const DupGroupCallback& cb)
{
    DupGroup group;
    std::vector<DupEntry> entries;

    for (const auto& grp : groupVec)
    {
        entries.clear();
        for (const auto& p : grp)
        {
            entries.emplace_back(p, 0, "n/a");
        }

        group.groupId = 0; // doesn't matter while emulating
        group.entires = entries;

        cb(group);
    }
}

} // namespace

TEST(DuplicateDeletionTest, IgnoreFilesModule)
{
    SilenceLogger silenceLogger;
    file::TempDir tmp("ignored");
    const auto filePath(tmp.path() / "ignored.txt");

    {
        // Tests that nothing is created if there is no file
        IgnoredPaths ignoredPaths(filePath, true);
        EXPECT_EQ(0, ignoredPaths.files().size());
    }
    EXPECT_FALSE(fs::exists(filePath));

    {
        IgnoredPaths ignoredPaths(filePath, true);
        EXPECT_EQ(0, ignoredPaths.files().size());
        ignoredPaths.add("file.dat");
        EXPECT_EQ(1, ignoredPaths.files().size());
    }

    {
        // Should load previously saved file
        IgnoredPaths ignoredPaths(filePath, true);
        EXPECT_EQ(1, ignoredPaths.files().size());
        EXPECT_EQ("file.dat", *ignoredPaths.files().begin());
    }

    {
        // Provide invalid file path
        IgnoredPaths ignoredPaths(tmp.path() / "file_.txt/", true);
        ignoredPaths.add("file.dat");
    }
}

TEST(DuplicateDeletionTest, DeleteFiles)
{
    PathsVec files {"file1.txt", "file2.txt", "file3.txt"};
    PathsVec deleted;
    PathsSet ignoredPaths;
    MockDelete strategy;

    EXPECT_CALL(strategy, remove(testing::_))
        .Times(static_cast<int>(files.size()))
        .WillRepeatedly([&deleted](const fs::path& p) {
            deleted.push_back(p);
        });

    EXPECT_NO_THROW(deleteFiles(strategy, files));
    EXPECT_TRUE(files.empty());
    EXPECT_EQ(deleted.size(), 3);
    EXPECT_EQ(deleted[0], "file1.txt");
    EXPECT_EQ(deleted[1], "file2.txt");
    EXPECT_EQ(deleted[2], "file3.txt");
}

TEST(DuplicateDeletionTest, DeleteFilesException)
{
    TestStLogInterceptor logInterceptor;

    PathsVec files {"file1.txt", "file2.txt", "file3.txt"};
    MockDelete strategy;
    EXPECT_CALL(strategy, remove(testing::_))
        .Times(static_cast<int>(files.size()))
        .WillOnce(testing::Throw(std::runtime_error("Deletion error")))
        .WillRepeatedly(testing::Return());

    EXPECT_NO_THROW(deleteFiles(strategy, files));
    EXPECT_TRUE(logInterceptor.count() > 0);
    EXPECT_TRUE(logInterceptor.contains(
        "Error 'Deletion error' while deleting file 'file1.txt'"));
}


TEST(DuplicateDeletionTest, DeleteFilesInteractively_KeepSecond)
{
    PathsVec files {"file1.txt", "file2.txt", "file3.txt"};
    PathsVec deleted;
    IgnoredPaths ignored;
    KeepFromPaths keepFrom;
    MockDelete strategy;

    EXPECT_CALL(strategy, remove(testing::_))
        .Times(2)
        .WillRepeatedly([&deleted](const fs::path& p) {
            deleted.push_back(p);
        });

    std::ostringstream out;
    std::istringstream in("2\n"); // Simulate user input to keep the second file

    EXPECT_TRUE(deleteInteractively(strategy, files, ignored, keepFrom, out, in));
    EXPECT_TRUE(ignored.empty());
    ASSERT_TRUE(files.empty());
    ASSERT_EQ(deleted.size(), 2);
    ASSERT_EQ(deleted[0], "file1.txt");
    ASSERT_EQ(deleted[1], "file3.txt");
}


TEST(DuplicateDeletionTest, DeleteFilesInteractively_KeepPaths_OneMatch)
{
    PathsVec files {"keep/file1.txt", "file2.txt", "file3.txt"};
    PathsVec deleted;
    IgnoredPaths ignored;
    KeepFromPaths keepFrom;
    MockDelete strategy;

    keepFrom.add(fs::path {"keep"});

    EXPECT_CALL(strategy, remove(testing::_))
        .Times(2)
        .WillRepeatedly([&deleted](const fs::path& p) {
            deleted.push_back(p);
        });

    std::ostringstream out;
    std::istringstream in; // No user input should be needed, as keep path will resolve

    EXPECT_TRUE(deleteInteractively(strategy, files, ignored, keepFrom, out, in));
    EXPECT_TRUE(ignored.empty());
    ASSERT_TRUE(files.empty());
    ASSERT_EQ(deleted.size(), 2);
    ASSERT_EQ(deleted[0], "file3.txt");
    ASSERT_EQ(deleted[1], "file2.txt");
}


TEST(DuplicateDeletionTest, DeleteFilesInteractively_KeepPaths_MultipleMatches)
{
    PathsVec files {"keep/file1.txt", "keep/file2.txt", "file3.txt"};
    PathsVec deleted;
    IgnoredPaths ignored;
    KeepFromPaths keepFrom;
    MockDelete strategy;

    keepFrom.add(fs::path {"keep"});

    EXPECT_CALL(strategy, remove(testing::_))
        .Times(2)
        .WillRepeatedly([&deleted](const fs::path& p) {
            deleted.push_back(p);
        });

    std::ostringstream out;
    std::istringstream in("3\n"); // User instruct to keep file3.txt

    EXPECT_TRUE(deleteInteractively(strategy, files, ignored, keepFrom, out, in));
    EXPECT_TRUE(ignored.empty());
    ASSERT_TRUE(files.empty());
    ASSERT_EQ(deleted.size(), 2);
    ASSERT_EQ(deleted[0], "keep/file1.txt");
    ASSERT_EQ(deleted[1], "keep/file2.txt");
}


TEST(DuplicateDeletionTest, DeleteFilesInteractively_ConsecutiveCalls)
{
    PathsVec files {"file1.txt", "file2.txt", "file3.txt"};
    PathsVec filesCopy = files;
    IgnoredPaths ignored;
    KeepFromPaths keepFrom;
    MockDelete strategy;

    // 2 calls per deleteInteractively call
    EXPECT_CALL(strategy, remove(testing::_))
        .Times(4)
        .WillRepeatedly([](const fs::path& p) {
            EXPECT_TRUE(p.string() != "file1.txt");
        });

    std::ostringstream out;
    std::istringstream in("1\n\n"); // Keeps the first file during each call

    EXPECT_TRUE(deleteInteractively(strategy, files, ignored, keepFrom, out, in));
    EXPECT_TRUE(deleteInteractively(strategy, filesCopy, ignored, keepFrom, out, in));
    EXPECT_TRUE(ignored.empty());
    ASSERT_TRUE(files.empty());
    ASSERT_TRUE(filesCopy.empty());
}

TEST(DuplicateDeletionTest, DeleteFilesInteractively_IgnoreGroup)
{
    SilenceLogger silenceLogger;
    PathsVec files {"file1.txt", "file2.txt", "file3.txt"};
    IgnoredPaths ignored;
    KeepFromPaths keepFrom;
    MockDelete strategy;

    EXPECT_CALL(strategy, remove(testing::_)).Times(0);

    std::ostringstream out;
    std::istringstream in("i\n"); // Simulate user input to keep the second file

    EXPECT_TRUE(deleteInteractively(strategy, files, ignored, keepFrom, out, in));
    ASSERT_EQ(files.size(), 3);
    ASSERT_EQ(ignored.size(), 3);
    EXPECT_TRUE(ignored.contains(files[0]));
    EXPECT_TRUE(ignored.contains(files[1]));
    EXPECT_TRUE(ignored.contains(files[2]));
}

TEST(DuplicateDeletionTest, DeleteFilesInteractively_InterruptDeletion)
{
    SilenceLogger silenceLogger;
    PathsVec files {"file1.txt", "file2.txt"};
    IgnoredPaths ignored;
    KeepFromPaths keepFrom;
    MockDelete strategy;

    EXPECT_CALL(strategy, remove(testing::_)).Times(0);

    std::ostringstream out;
    std::istringstream in("q"); // Simulate user input to keep the second file

    EXPECT_FALSE(deleteInteractively(strategy, files, ignored, keepFrom, out, in));
    ASSERT_EQ(files.size(), 2);
    ASSERT_EQ(ignored.size(), 0);
}

TEST(DuplicateDeletionTest, DeleteFilesInteractively_InvalidChoice)
{
    PathsVec files {"file1.txt"};
    IgnoredPaths ignored;
    KeepFromPaths keepFrom;
    MockDelete strategy;

    EXPECT_CALL(strategy, remove(testing::_)).Times(0);

    std::ostringstream out;
    std::istringstream in("4\nW\n"); // Invalid choice

    EXPECT_FALSE(deleteInteractively(strategy, files, ignored, keepFrom, out, in));
    ASSERT_EQ(files.size(), 1);
    ASSERT_TRUE(ignored.empty());
    EXPECT_TRUE(!in); // input should be fully consumed
}

TEST(DuplicateDeletionTest, DeleteFilesInteractively_BadStream)
{
    SilenceLogger silenceLogger;
    PathsVec files {"file1.txt"};
    IgnoredPaths ignored;
    KeepFromPaths keepFrom;
    MockDelete strategy;

    EXPECT_CALL(strategy, remove(testing::_)).Times(0);

    std::ostringstream out;
    std::istringstream in; // no input

    EXPECT_FALSE(deleteInteractively(strategy, files, ignored, keepFrom, out, in));
    ASSERT_EQ(files.size(), 1);
    ASSERT_TRUE(ignored.empty());
}


TEST(DuplicateDeletionTest, DeleteDuplicates_ExpectedFilesInSafeDirs)
{
    MockDeletionStrategy strategy;
    MockDuplicateGroups groups;
    DeleteFromPaths deleteFrom;

    deleteFrom.add(fs::path {"safeDir"});

    // Two groups, each with two files in safeDir
    std::vector<PathsVec> groupVec {
        {fs::path("origDir/file1.txt"), fs::path("safeDir/file2.txt")},
        {fs::path("origDir/file3.txt"), fs::path("safeDir/file4.txt")}};

    EXPECT_CALL(groups, numGroups()).Times(1);
    EXPECT_CALL(groups, enumGroups(testing::_))
        .WillOnce([&groupVec](const DupGroupCallback& cb) {
            emulateDupGroups(groupVec, cb);
        });

    // Only the second file in each group should be deleted
    EXPECT_CALL(strategy, remove(fs::path("safeDir/file2.txt"))).Times(1);
    EXPECT_CALL(strategy, remove(fs::path("safeDir/file4.txt"))).Times(1);

    std::ostringstream out;
    std::istringstream in;

    DeletionConfig cfg {groups, strategy, out, in};
    cfg.setDeleteFromPaths(deleteFrom);

    deleteDuplicates(cfg);
}

TEST(DuplicateDeletionTest, DeleteDuplicates_ExpectedFilesAllInSafeDirs)
{
    MockDeletionStrategy strategy;
    MockDuplicateGroups groups;
    DeleteFromPaths deleteFrom;

    deleteFrom.add(fs::path {"safeDir"});

    // Two groups, each with two files in safeDir
    std::vector<PathsVec> groupVec {
        {fs::path("safeDir/file1.txt"), fs::path("safeDir/file2.txt")},
        {fs::path("safeDir/file3.txt"), fs::path("safeDir/file4.txt")}};

    EXPECT_CALL(groups, numGroups()).Times(1);
    EXPECT_CALL(groups, enumGroups(testing::_))
        .WillOnce([&groupVec](const DupGroupCallback& cb) {
            emulateDupGroups(groupVec, cb);
        });

    // Only the second file in each group should be deleted
    EXPECT_CALL(strategy, remove(fs::path("safeDir/file2.txt"))).Times(1);
    EXPECT_CALL(strategy, remove(fs::path("safeDir/file4.txt"))).Times(1);

    std::ostringstream out;
    // Need to perform selection, as both candidates are from the directory to
    // delete from
    std::istringstream in("1\n1\n");

    DeletionConfig cfg {groups, strategy, out, in};
    cfg.setDeleteFromPaths(deleteFrom);

    deleteDuplicates(cfg);
}

TEST(DuplicateDeletionTest, DeleteDuplicates_ExpectedFilesSelectively)
{
    MockDeletionStrategy strategy;
    MockDuplicateGroups groups;

    // Two groups, each with two files in safeDir
    std::vector<PathsVec> groupVec {
        {fs::path("origDir/file1.txt"), fs::path("dupDir/file2.txt")},
        {fs::path("origDir/file3.txt"), fs::path("dupDir/file4.txt")}};

    EXPECT_CALL(groups, numGroups()).Times(1);
    EXPECT_CALL(groups, enumGroups(testing::_))
        .WillOnce([&groupVec](const DupGroupCallback& cb) {
            emulateDupGroups(groupVec, cb);
        });

    // Only the second file in each group should be deleted
    EXPECT_CALL(strategy, remove(fs::path("dupDir/file2.txt"))).Times(1);
    EXPECT_CALL(strategy, remove(fs::path("dupDir/file4.txt"))).Times(1);

    std::ostringstream out;

    // keep the second file
    // paths are listed in a sorted order, thus dupDir* comes before origDir*
    // in other words we keep second file (i.e. orig file), that's why we specify 2,2
    std::istringstream in("2\n2\n");

    DeletionConfig cfg {groups, strategy, out, in};
    deleteDuplicates(cfg);
}

TEST(DuplicateDeletionTest, DeleteDuplicates_SkipsIgnoredFiles)
{
    SilenceLogger silenceLogger;
    MockDeletionStrategy strategy;
    MockDuplicateGroups groups;
    IgnoredPaths ignored;
    DeleteFromPaths deleteFrom;

    // Mark file2.txt as ignored
    ignored.add(fs::path("safeDir/file2.txt"));
    deleteFrom.add(fs::path {"safeDir"});

    std::vector<PathsVec> groupVec = {
        {fs::path("OrigDir/file1.txt"), fs::path("safeDir/file2.txt")}};

    EXPECT_CALL(groups, numGroups()).Times(1);
    EXPECT_CALL(groups, enumGroups(testing::_))
        .WillOnce([&groupVec](const DupGroupCallback& cb) {
            emulateDupGroups(groupVec, cb);
        });
    // Only file2.txt would be deleted, but it's ignored, so remove should not be
    // called
    EXPECT_CALL(strategy, remove(testing::_)).Times(0);

    std::ostringstream out;
    std::istringstream in;

    DeletionConfig cfg {groups, strategy, out, in};
    cfg.setIgnoredPaths(ignored);
    cfg.setDeleteFromPaths(deleteFrom);

    deleteDuplicates(cfg);
}

} // namespace tools::dups
