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
    MOCK_METHOD(void, apply, (const fs::path& file), (const, override));
};


class MockDeletionStrategy : public IDeletionStrategy {
public:
    MOCK_METHOD(void, apply, (const fs::path&), (const, override));
};


class MockDuplicateGroups : public IDuplicateGroups {
public:
    MOCK_METHOD((size_t), numGroups, (), (const, noexcept, override));
    MOCK_METHOD((void), enumGroups, (const DupGroupCallback&), (const, override));
};

void emulateDupGroups(const std::vector<PathsVec>& groupVec, const DupGroupCallback& cb)
{
    DupGroup group;
    std::vector<DupEntry> entries;

    for (const auto& grp: groupVec)
    {
        entries.clear();
        for (const auto& p: grp)
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
        IgnoredFiles ignoredFiles(filePath, true);
        EXPECT_EQ(0, ignoredFiles.files().size());
    }
    EXPECT_FALSE(fs::exists(filePath));

    {
        IgnoredFiles ignoredFiles(filePath, true);
        EXPECT_EQ(0, ignoredFiles.files().size());
        ignoredFiles.add("file.dat");
        EXPECT_EQ(1, ignoredFiles.files().size());
    }

    {
        // Should load previously saved file
        IgnoredFiles ignoredFiles(filePath, true);
        EXPECT_EQ(1, ignoredFiles.files().size());
        EXPECT_EQ("file.dat", *ignoredFiles.files().begin());
    }

    {
        // Provide invalid file path
        IgnoredFiles ignoredFiles(tmp.path() / "file_.txt/", true);
        ignoredFiles.add("file.dat");
    }
}

TEST(DuplicateDeletionTest, DeleteFiles)
{
    PathsVec files {
        "file1.txt",
        "file2.txt",
        "file3.txt"
    };
    PathsVec deleted;
    PathsSet ignoredFiles;
    MockDelete strategy;

    EXPECT_CALL(strategy, apply(testing::_))
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

    PathsVec files {
        "file1.txt",
        "file2.txt",
        "file3.txt"
    };
    MockDelete strategy;
    EXPECT_CALL(strategy, apply(testing::_))
        .Times(static_cast<int>(files.size()))
        .WillOnce(testing::Throw(std::runtime_error("Deletion error")))
        .WillRepeatedly(testing::Return());

    EXPECT_NO_THROW(deleteFiles(strategy, files));
    EXPECT_TRUE(logInterceptor.count() > 0);
    EXPECT_TRUE(logInterceptor.contains("Error 'Deletion error' while deleting file 'file1.txt'"));
}


TEST(DuplicateDeletionTest, DeleteFilesInteractively_KeepSecond)
{
    PathsVec files {
        "file1.txt",
        "file2.txt",
        "file3.txt"
    };
    PathsVec deleted;
    IgnoredFiles ignoredFiles;
    MockDelete strategy;

    EXPECT_CALL(strategy, apply(testing::_))
        .Times(2)
        .WillRepeatedly([&deleted](const fs::path& p) {
            deleted.push_back(p);
        });

    std::ostringstream out;
    std::istringstream in("2\n"); // Simulate user input to keep the second file

    EXPECT_TRUE(deleteInteractively(strategy, files, ignoredFiles, out, in));
    EXPECT_TRUE(ignoredFiles.empty());
    ASSERT_TRUE(files.empty());
    ASSERT_EQ(deleted.size(), 2);
    ASSERT_EQ(deleted[0], "file1.txt");
    ASSERT_EQ(deleted[1], "file3.txt");
}

TEST(DuplicateDeletionTest, DeleteFilesInteractively_ConsecutiveCalls)
{
    PathsVec files {
        "file1.txt",
        "file2.txt",
        "file3.txt"
    };
    PathsVec filesCopy = files;
    IgnoredFiles ignoredFiles;
    MockDelete strategy;

    // 2 calls per deleteInteractively call
    EXPECT_CALL(strategy, apply(testing::_)).Times(4)
        .WillRepeatedly([](const fs::path& p) {
            EXPECT_TRUE(p.string() != "file1.txt");
        });

    std::ostringstream out;
    std::istringstream in("1\n\n"); // Keeps the first file during each call

    EXPECT_TRUE(deleteInteractively(strategy, files, ignoredFiles, out, in));
    EXPECT_TRUE(deleteInteractively(strategy, filesCopy, ignoredFiles, out, in));
    EXPECT_TRUE(ignoredFiles.empty());
    ASSERT_TRUE(files.empty());
    ASSERT_TRUE(filesCopy.empty());
}

TEST(DuplicateDeletionTest, DeleteFilesInteractively_IgnoreGroup)
{
    SilenceLogger silenceLogger;
    PathsVec files {
        "file1.txt",
        "file2.txt",
        "file3.txt"
    };
    IgnoredFiles ignoredFiles;
    MockDelete strategy;

    EXPECT_CALL(strategy, apply(testing::_)).Times(0);

    std::ostringstream out;
    std::istringstream in("i\n"); // Simulate user input to keep the second file

    EXPECT_TRUE(deleteInteractively(strategy, files, ignoredFiles, out, in));
    ASSERT_EQ(files.size(), 3);
    ASSERT_EQ(ignoredFiles.size(), 3);
    EXPECT_TRUE(ignoredFiles.contains(files[0]));
    EXPECT_TRUE(ignoredFiles.contains(files[1]));
    EXPECT_TRUE(ignoredFiles.contains(files[2]));
}

TEST(DuplicateDeletionTest, DeleteFilesInteractively_QuitDeletion)
{
    SilenceLogger silenceLogger;
    PathsVec files {
        "file1.txt",
        "file2.txt"
    };
    IgnoredFiles ignoredFiles;
    MockDelete strategy;

    EXPECT_CALL(strategy, apply(testing::_)).Times(0);

    std::ostringstream out;
    std::istringstream in("q"); // Simulate user input to keep the second file

    EXPECT_FALSE(deleteInteractively(strategy, files, ignoredFiles, out, in));
    ASSERT_EQ(files.size(), 2);
    ASSERT_EQ(ignoredFiles.size(), 0);
}

TEST(DuplicateDeletionTest, DeleteFilesInteractively_InvalidChoice)
{
    PathsVec files {"file1.txt"};
    IgnoredFiles ignoredFiles;
    MockDelete strategy;

    EXPECT_CALL(strategy, apply(testing::_)).Times(0);

    std::ostringstream out;
    std::istringstream in("4\nW\n"); // Invalid choice

    EXPECT_TRUE(deleteInteractively(strategy, files, ignoredFiles, out, in));
    EXPECT_TRUE(deleteInteractively(strategy, files, ignoredFiles, out, in));
    ASSERT_EQ(files.size(), 1);
    ASSERT_TRUE(ignoredFiles.empty());
}

TEST(DuplicateDeletionTest, DeleteFilesInteractively_BadStream)
{
    SilenceLogger silenceLogger;
    PathsVec files {"file1.txt"};
    IgnoredFiles ignoredFiles;
    MockDelete strategy;

    EXPECT_CALL(strategy, apply(testing::_)).Times(0);

    std::ostringstream out;
    std::istringstream in; // no input

    EXPECT_FALSE(deleteInteractively(strategy, files, ignoredFiles, out, in));
    ASSERT_EQ(files.size(), 1);
    ASSERT_TRUE(ignoredFiles.empty());
}


TEST(DuplicateDeletionTest, DeleteDuplicates_ExpectedFilesInSafeDirs)
{
    MockDeletionStrategy strategy;
    MockDuplicateGroups groups;
    IgnoredFiles ignored;
    PathsVec safeToDeleteDirs = {"safeDir"};
    Progress progress;

    // Two groups, each with two files in safeDir
    std::vector<PathsVec> groupVec
    {
        {fs::path("origDir/file1.txt"), fs::path("safeDir/file2.txt")},
        {fs::path("origDir/file3.txt"), fs::path("safeDir/file4.txt")}
    };

    EXPECT_CALL(groups, numGroups()).Times(1);
    EXPECT_CALL(groups, enumGroups(testing::_))
        .WillOnce([&groupVec](const DupGroupCallback& cb) {
            emulateDupGroups(groupVec, cb);
        });

    // Only the second file in each group should be deleted
    EXPECT_CALL(strategy, apply(fs::path("safeDir/file2.txt"))).Times(1);
    EXPECT_CALL(strategy, apply(fs::path("safeDir/file4.txt"))).Times(1);

    std::ostringstream out;
    std::istringstream in;

    deleteDuplicates(strategy, groups, safeToDeleteDirs, ignored, progress, out, in);
}

TEST(DuplicateDeletionTest, DeleteDuplicates_ExpectedFilesAllInSafeDirs)
{
    MockDeletionStrategy strategy;
    MockDuplicateGroups groups;
    IgnoredFiles ignored;
    PathsVec safeToDeleteDirs = {"safeDir"};
    Progress progress;

    // Two groups, each with two files in safeDir
    std::vector<PathsVec> groupVec
    {
        {fs::path("safeDir/file1.txt"), fs::path("safeDir/file2.txt")},
        {fs::path("safeDir/file3.txt"), fs::path("safeDir/file4.txt")}
    };

    EXPECT_CALL(groups, numGroups()).Times(1);
    EXPECT_CALL(groups, enumGroups(testing::_))
        .WillOnce([&groupVec](const DupGroupCallback& cb) {
            emulateDupGroups(groupVec, cb);
        });

    // Only the second file in each group should be deleted
    EXPECT_CALL(strategy, apply(fs::path("safeDir/file2.txt"))).Times(1);
    EXPECT_CALL(strategy, apply(fs::path("safeDir/file4.txt"))).Times(1);

    std::ostringstream out;
    // Need to perform selection, as both candidates are from the safe to
    // delete group
    std::istringstream in("1\n1\n");

    deleteDuplicates(strategy, groups, safeToDeleteDirs, ignored, progress, out, in);
}

TEST(DuplicateDeletionTest, DeleteDuplicates_ExpectedFilesSelectively)
{
    MockDeletionStrategy strategy;
    MockDuplicateGroups groups;
    IgnoredFiles ignored;
    PathsVec safeToDeleteDirs;
    Progress progress;

    // Two groups, each with two files in safeDir
    std::vector<PathsVec> groupVec
    {
        {fs::path("origDir/file1.txt"), fs::path("dupDir/file2.txt")},
        {fs::path("origDir/file3.txt"), fs::path("dupDir/file4.txt")}
    };

    EXPECT_CALL(groups, numGroups()).Times(1);
    EXPECT_CALL(groups, enumGroups(testing::_))
        .WillOnce([&groupVec](const DupGroupCallback& cb) {
            emulateDupGroups(groupVec, cb);
        });

    // Only the second file in each group should be deleted
    EXPECT_CALL(strategy, apply(fs::path("dupDir/file2.txt"))).Times(1);
    EXPECT_CALL(strategy, apply(fs::path("dupDir/file4.txt"))).Times(1);

    std::ostringstream out;

    // keep the second file
    // paths are listed in a sorted order, thus dupDir* comes before origDir*
    // in other words we keep second file (i.e. orig file), that's why we specify 2,2
    std::istringstream in("2\n2\n");

    deleteDuplicates(strategy, groups, safeToDeleteDirs, ignored, progress, out, in);
}

TEST(DuplicateDeletionTest, DeleteDuplicates_SkipsIgnoredFiles)
{
    SilenceLogger silenceLogger;
    MockDeletionStrategy strategy;
    MockDuplicateGroups groups;
    IgnoredFiles ignored;
    PathsVec safeToDeleteDirs = {"safeDir"};
    Progress progress;

    // Mark file2.txt as ignored
    ignored.add(fs::path("safeDir/file2.txt"));

    std::vector<PathsVec> groupVec = {
        {fs::path("OrigDir/file1.txt"), fs::path("safeDir/file2.txt")}
    };

    EXPECT_CALL(groups, numGroups()).Times(1);
    EXPECT_CALL(groups, enumGroups(testing::_))
        .WillOnce([&groupVec](const DupGroupCallback& cb) {
            emulateDupGroups(groupVec, cb);
        });
    // Only file2.txt would be deleted, but it's ignored, so apply should not be called
    EXPECT_CALL(strategy, apply(testing::_)).Times(0);

    std::ostringstream out;
    std::istringstream in;

    deleteDuplicates(strategy, groups, safeToDeleteDirs, ignored, progress, out, in);
}

} // namespace tools::dups
