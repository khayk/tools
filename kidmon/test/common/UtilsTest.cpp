#include <gtest/gtest.h>
#include <kidmon/common/Utils.h>

#include <algorithm>
#include <cctype>

TEST(UtilsTest, GenerateToken)
{
    const auto actual = utl::generateToken(42);

    EXPECT_EQ(42, actual.size());
    std::ranges::for_each(actual, [](const char ch) {
        EXPECT_TRUE(std::isalnum(ch));
    });
}