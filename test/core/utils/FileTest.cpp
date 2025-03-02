#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <core/utils/File.h>

#include <filesystem>

namespace fs = std::filesystem;
using testing::_;
using testing::MockFunction;
using namespace file;

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

TEST_F(UtilsFileTests, ReadLines)
{
    const fs::path file = testDir_ / "data.txt";
    write(file, "one\ntwo\n\nfour");

    std::vector<std::string> lines;
    std::error_code ec;

    EXPECT_TRUE(readLines(file, [&lines](const std::string& line) {
        lines.push_back(line);
        return true;
    }, ec));

    EXPECT_TRUE(!ec);
    EXPECT_EQ(4,      lines.size());
    EXPECT_EQ("one",  lines[0]);
    EXPECT_EQ("two",  lines[1]);
    EXPECT_EQ("",     lines[2]);
    EXPECT_EQ("four", lines[3]);

    lines.clear();
    EXPECT_FALSE(readLines(file, [&lines](const std::string& line) {
        lines.push_back(line);
        return false;
    }, ec));
    // We have instructed to stop enumeration after the first entry
    EXPECT_EQ(1,      lines.size());
}

TEST_F(UtilsFileTests, ReadLinesFails)
{
    const fs::path file = testDir_ / "data.txt";
    std::error_code ec;

    MockFunction<void(const std::string&)> mock;
    EXPECT_CALL(mock, Call(_)).Times(0);

    auto lambda = [&mock](const std::string& line) { mock.Call(line); return true; };
    EXPECT_FALSE(readLines(file, lambda, ec));
    EXPECT_FALSE(!ec);
}

} // namespace
