#include <gtest/gtest.h>
#include <kidmon/common/Utils.h>

#include <algorithm>
#include <cctype>
#include <unordered_set>

using namespace km;

TEST(UtilsTest, GenerateToken)
{
    const auto actual = utl::generateToken(42);

    EXPECT_EQ(42, actual.size());
    std::ranges::for_each(actual, [](const char ch) {
        EXPECT_TRUE(std::isalnum(ch));
    });
}

TEST(UtilsTest, GenerateTokenRespectsLength)
{
    EXPECT_TRUE(utl::generateToken(0).empty());
    EXPECT_EQ(1, utl::generateToken(1).size());
    EXPECT_EQ(256, utl::generateToken(256).size());
}

TEST(UtilsTest, GenerateTokenIsNotConstant)
{
    // A secure generator must not return the same secret twice in practice.
    // Generate a batch and assert they are all distinct.
    std::unordered_set<std::string> tokens;
    for (int i = 0; i < 100; ++i)
    {
        tokens.insert(utl::generateToken(16));
    }

    EXPECT_EQ(100, tokens.size());
}