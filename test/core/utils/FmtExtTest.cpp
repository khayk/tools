#include <gtest/gtest.h>
#include <core/utils/FmtExt.h>
#include <filesystem>

namespace {

TEST(UtilsFmtExtTests, Formatting)
{
    fs::path testPath = "file.txt";
    EXPECT_EQ("file.txt", fmt::format("{}", testPath));
}

} // namespace