#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DeletionStrategy.h>
#include <core/utils/Log.h>
#include <core/utils/LogInterceptor.h>

using testing::MockFunction;
using testing::Return;

namespace tools::dups {

using utl::SilenceLogger;
using utl::TestStLogInterceptor;

namespace {

class MockDelete : public IDeletionStrategy
{
public:
    MOCK_METHOD(void, apply, (const fs::path& file), (override));
};

} // namespace

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

} // namespace tools::dups
