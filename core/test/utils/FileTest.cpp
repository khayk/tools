#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <core/utils/File.h>
#include "utils/FileDetail.h"

#include <exception>
#include <filesystem>
#include <array>
#include <regex>

namespace fs = std::filesystem;
using testing::_;
using testing::HasSubstr;
using testing::MockFunction;
using testing::MatchesRegex;
using namespace core::file;

namespace {

class UtilsFileTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        fs::create_directories(testDir_);
    }

    void TearDown() override
    {
        fs::remove_all(testDir_);
    }

protected:
    const fs::path testDir_ {"file_testing"};
};

// ---------------------------------------------------------------------------
// write / append / read
// ---------------------------------------------------------------------------

TEST_F(UtilsFileTests, Write)
{
    const fs::path filename = testDir_ / "data1.txt";
    write(filename, "One line for data");

    ASSERT_TRUE(fs::exists(filename));
    std::string content;
    std::error_code ec;

    ASSERT_TRUE(read(filename, content, ec));
    ASSERT_FALSE(ec);
    EXPECT_EQ(content, "One line for data");
}

TEST_F(UtilsFileTests, WriteRawPointer)
{
    const fs::path filename = testDir_ / "raw.txt";
    const std::string data {"raw data"};
    write(filename, data.data(), data.size());

    std::string content;
    std::error_code ec;
    ASSERT_TRUE(read(filename, content, ec));
    EXPECT_EQ("raw data", content);
}

TEST_F(UtilsFileTests, AppendCreatesFile)
{
    const fs::path filename = testDir_ / "append.txt";
    append(filename, "hello");

    std::string content;
    std::error_code ec;
    ASSERT_TRUE(read(filename, content, ec));
    EXPECT_EQ("hello", content);
}

TEST_F(UtilsFileTests, AppendToExisting)
{
    const fs::path filename = testDir_ / "append.txt";
    write(filename, "hello");
    append(filename, " world");

    std::string content;
    std::error_code ec;
    ASSERT_TRUE(read(filename, content, ec));
    EXPECT_EQ("hello world", content);
}

TEST_F(UtilsFileTests, AppendRawPointer)
{
    const fs::path filename = testDir_ / "append_raw.txt";
    const char data[] = "raw";
    append(filename, data, sizeof(data) - 1);

    std::string content;
    std::error_code ec;
    ASSERT_TRUE(read(filename, content, ec));
    EXPECT_EQ("raw", content);
}

TEST_F(UtilsFileTests, ReadMissingFile)
{
    std::string content;
    std::error_code ec;
    EXPECT_FALSE(read(testDir_ / "missing.txt", content, ec));
    EXPECT_TRUE(ec.operator bool());
}

// ---------------------------------------------------------------------------
// readLines
// ---------------------------------------------------------------------------

TEST_F(UtilsFileTests, ReadLines)
{
    const fs::path file = testDir_ / "data.txt";
    write(file, "one\ntwo\n\nfour");

    std::vector<std::string> lines;

    EXPECT_NO_THROW(readLines(file, [&lines](const std::string& line) {
        lines.push_back(line);
        return true;
    }));

    ASSERT_EQ(4, lines.size());
    EXPECT_EQ("one", lines[0]);
    EXPECT_EQ("two", lines[1]);
    EXPECT_EQ("", lines[2]);
    EXPECT_EQ("four", lines[3]);

    lines.clear();
    EXPECT_NO_THROW(readLines(file, [&lines](const std::string& line) {
        lines.push_back(line);
        return false;
    }));
    // We have instructed to stop enumeration after the first entry
    EXPECT_EQ(1, lines.size());
}

TEST_F(UtilsFileTests, ReadLinesEmptyFile)
{
    const fs::path file = testDir_ / "data.txt";
    write(file, "");

    std::vector<std::string> lines;

    EXPECT_NO_THROW(readLines(file, [&lines](const std::string& line) {
        lines.push_back(line);
        return true;
    }));

    ASSERT_EQ(0, lines.size());
}

TEST_F(UtilsFileTests, ReadLinesFails)
{
    const fs::path file = testDir_ / "data.txt";

    MockFunction<void(const std::string&)> mock;
    EXPECT_CALL(mock, Call(_)).Times(0);

    auto lambda = [&mock](const std::string& line) {
        mock.Call(line);
        return true;
    };
    EXPECT_THROW(readLines(file, lambda), std::exception);
}

// ---------------------------------------------------------------------------
// path2s
// ---------------------------------------------------------------------------

TEST(UtilsFilePathConvTests, Path2s)
{
    EXPECT_EQ("/tmp/file.txt", path2s(fs::path("/tmp/file.txt")));
    EXPECT_EQ("", path2s(fs::path()));
}

// ---------------------------------------------------------------------------
// constructTempPath
// ---------------------------------------------------------------------------

TEST(UtilsFileTempPathTests, DefaultPrefix)
{
    const fs::path p = constructTempPath("", fs::temp_directory_path());
    EXPECT_TRUE(fs::equivalent(p.parent_path(), fs::temp_directory_path()));
    EXPECT_THAT(p.filename().string(), MatchesRegex("tmp-[0-9]+-[a-z]{6}"));
}

TEST(UtilsFileTempPathTests, WithPrefix)
{
    const fs::path p = constructTempPath("myapp", fs::temp_directory_path());
    EXPECT_THAT(p.filename().string(), MatchesRegex("myapp-[0-9]+-[a-z]{6}"));
}

TEST(UtilsFileTempPathTests, Unique)
{
    const fs::path p1 = constructTempPath("", fs::temp_directory_path());
    const fs::path p2 = constructTempPath("", fs::temp_directory_path());
    EXPECT_NE(p1, p2);
}

TEST_F(UtilsFileTests, ConstructTempPath_CustomDir)
{
    const fs::path p = constructTempPath("", testDir_);
    EXPECT_EQ(p.parent_path().lexically_normal(), testDir_.lexically_normal());
}

TEST(UtilsFileTempPathTests, DoesNotCreatePath)
{
    const fs::path p = constructTempPath("", fs::temp_directory_path());
    EXPECT_FALSE(fs::exists(p));
}

// ---------------------------------------------------------------------------
// Path / ScopedDir
// ---------------------------------------------------------------------------

TEST(UtilsFileScopedDirTests, DefaultIsNotOwner)
{
    ScopedDir d;
    EXPECT_FALSE(d.isOwner());
    EXPECT_TRUE(d.path().empty());
}

TEST_F(UtilsFileTests, ScopedDirOwnsPath)
{
    ScopedDir d(testDir_ / "sub");
    EXPECT_TRUE(d.isOwner());
    EXPECT_EQ(d.path(), testDir_ / "sub");
}

TEST_F(UtilsFileTests, DropOwnership)
{
    ScopedDir d(testDir_ / "sub");
    d.dropOwnership();
    EXPECT_FALSE(d.isOwner());
}

TEST_F(UtilsFileTests, TakeOwnership)
{
    ScopedDir d;
    d.takeOwnership(testDir_ / "new");
    EXPECT_TRUE(d.isOwner());
    EXPECT_EQ(d.path(), testDir_ / "new");
}

TEST_F(UtilsFileTests, ScopedDirDeletesOnDestruction)
{
    const fs::path dir = testDir_ / "scoped";
    fs::create_directories(dir);
    {
        ScopedDir d(dir);
        EXPECT_TRUE(fs::exists(dir));
    }
    EXPECT_FALSE(fs::exists(dir));
}

TEST_F(UtilsFileTests, ScopedDirNoDeleteWhenNotOwner)
{
    const fs::path dir = testDir_ / "scoped";
    fs::create_directories(dir);
    {
        ScopedDir d(dir);
        d.dropOwnership();
    }
    EXPECT_TRUE(fs::exists(dir));
}

// ---------------------------------------------------------------------------
// TempDir
// ---------------------------------------------------------------------------

TEST(UtilsFileTempDirTests, AutoModeCreatesDirectory)
{
    fs::path p;
    {
        TempDir td("test");
        p = td.path();
        EXPECT_TRUE(fs::is_directory(p));
    }
    EXPECT_FALSE(fs::exists(p));
}

TEST(UtilsFileTempDirTests, ManualModeDoesNotCreate)
{
    TempDir td("test", TempDir::CreateMode::Manual);
    EXPECT_FALSE(fs::exists(td.path()));
}

TEST_F(UtilsFileTests, TempDirCustomDirectory)
{
    TempDir td("test", TempDir::CreateMode::Auto, testDir_);
    EXPECT_TRUE(fs::is_directory(td.path()));
    EXPECT_TRUE(td.path().string().starts_with(
        testDir_.lexically_normal().string()));
}

// ---------------------------------------------------------------------------
// enumFilesRecursive
// ---------------------------------------------------------------------------

TEST_F(UtilsFileTests, EnumFilesRecursive_Empty)
{
    std::vector<fs::path> found;
    enumFilesRecursive(testDir_, {}, [&](const fs::path& p, const std::error_code& ec) {
        if (!ec) {
            found.push_back(p);
        }
    });
    EXPECT_TRUE(found.empty());
}

TEST_F(UtilsFileTests, EnumFilesRecursive_ListsFiles)
{
    write(testDir_ / "a.txt", "a");
    write(testDir_ / "b.txt", "b");

    std::vector<fs::path> found;
    enumFilesRecursive(testDir_, {}, [&](const fs::path& p, const std::error_code& ec) {
        if (!ec) {
            found.push_back(p);
        }
    });
    EXPECT_EQ(2u, found.size());
}

TEST_F(UtilsFileTests, EnumFilesRecursive_Recursive)
{
    const fs::path sub = testDir_ / "sub";
    fs::create_directory(sub);
    write(sub / "c.txt", "c");

    std::vector<fs::path> found;
    enumFilesRecursive(testDir_, {}, [&](const fs::path& p, const std::error_code& ec) {
        if (!ec) {
            found.push_back(p);
        }
    });
    // sub/ directory + sub/c.txt
    EXPECT_EQ(2u, found.size());
}

TEST_F(UtilsFileTests, EnumFilesRecursive_ExclusionPattern)
{
    write(testDir_ / "a.txt", "a");
    write(testDir_ / "b.log", "b");

    std::vector<fs::path> found;
    enumFilesRecursive(testDir_,
                       {std::regex("\\.log$")},
                       [&](const fs::path& p, const std::error_code& ec) {
                           if (!ec) {
                               found.push_back(p);
                           }
                       });
    ASSERT_EQ(1u, found.size());
    EXPECT_EQ("a.txt", found[0].filename().string());
}

TEST(UtilsFileEnumTests, EnumFilesRecursive_NonExistentDir)
{
    bool gotError = false;
    enumFilesRecursive("/non/existent/path",
                       {},
                       [&](const fs::path&, const std::error_code& ec) {
                           if (ec) {
                               gotError = true;
                           }
                       });
    EXPECT_TRUE(gotError);
}

// ---------------------------------------------------------------------------
// detail::buildOpenDirCommand / detail::buildNavigateFileCommand
// ---------------------------------------------------------------------------

TEST_F(UtilsFileTests, BuildOpenDirCommand_ForDirectory)
{
    const auto cmd = detail::buildOpenDirCommand(testDir_);
    EXPECT_THAT(cmd, HasSubstr(path2s(testDir_)));
}

TEST_F(UtilsFileTests, BuildOpenDirCommand_ForFile)
{
    const fs::path file = testDir_ / "test.txt";
    write(file, "data");
    const auto cmd = detail::buildOpenDirCommand(file);
    // For a regular file the command targets the parent directory
    EXPECT_THAT(cmd, HasSubstr(path2s(testDir_)));
}

TEST(UtilsFileDetailTests, BuildOpenDirCommand_InvalidPathThrows)
{
    EXPECT_THROW(detail::buildOpenDirCommand("/non/existent/path"),
                 std::runtime_error);
}

TEST_F(UtilsFileTests, BuildNavigateFileCommand_ContainsPath)
{
    const fs::path file = testDir_ / "test.txt";
    write(file, "data");
    const auto cmd = detail::buildNavigateFileCommand(file);
    EXPECT_THAT(cmd, HasSubstr(path2s(file)));
}

} // namespace
