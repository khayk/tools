#include <gtest/gtest.h>
#include "utils/File.h"

#include <filesystem>

namespace fs = std::filesystem;
using namespace file;

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
