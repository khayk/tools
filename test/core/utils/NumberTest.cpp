#include <gtest/gtest.h>
#include <core/utils/Number.h>
#include <limits>

using namespace num;

namespace {

TEST(UtilsNumberTests, Number2String)
{
    const auto s1 = num2s(123);
    EXPECT_EQ(s1, "123");

    const auto s2 = num2s(123.456);
    EXPECT_EQ(s2, "123.456");

    const auto s3 = num2s(-789.012);
    EXPECT_EQ(s3, "-789.012");

    const auto s4 = num2s(0.0);
    EXPECT_EQ(s4, "0");

    const auto s5 = num2s(-0.0);
    EXPECT_EQ(s5, "-0");

    const auto s6 = num2s(1.2345678901234567890e10);
    EXPECT_EQ(s6, "");
}


TEST(UtilsNumberTests, Number2StringEdgeCases)
{
    EXPECT_EQ(num2s(0), "0");                                        // zero
    EXPECT_EQ(num2s(-0.0), "-0");                                    // negative zero
    EXPECT_EQ(num2s(1.2345678901234567890e10), "");                  // precision limit
    EXPECT_EQ(num2s(std::numeric_limits<int>::max()), "2147483647"); // max int
    EXPECT_EQ(num2s(std::numeric_limits<int>::min()), "-2147483648"); // min int
}


TEST(UtilsNumberTests, String2Number)
{
    EXPECT_EQ(s2num<size_t>("123"), 123);
    EXPECT_EQ(s2num<uint16_t>("987"), 987);

    EXPECT_EQ(s2num<int16_t>("32767"), 32'767);
    EXPECT_EQ(s2num<int16_t>("-32768"), -32'768);
    EXPECT_EQ(s2num<uint16_t>("65535"), 65'535);

    EXPECT_EQ(s2num<int32_t>("2147483647"), 2'147'483'647);
    EXPECT_EQ(s2num<int32_t>("-2147483648"), -2'147'483'648);
    EXPECT_EQ(s2num<uint32_t>("4294967295"), 4'294'967'295);

    EXPECT_EQ(s2num<int64_t>("9223372036854775807"), 9'223'372'036'854'775'807);
    EXPECT_EQ(s2num<int64_t>("-9223372036854775808"),
              -9'223'372'036'854'775'807LL - 1);
    EXPECT_EQ(s2num<uint64_t>("18446744073709551615"), 18'446'744'073'709'551'615ULL);

    EXPECT_EQ(s2num<int>("1234567890"), 1'234'567'890);

    EXPECT_EQ(s2num<double>("123.456789"), 123.456789);
    EXPECT_EQ(s2num<float>("987.654321"), 987.654321F);

    EXPECT_EQ(s2num<double>("-789.012"), -789.012);
    EXPECT_EQ(s2num<double>("0.0"), 0.0);
    EXPECT_EQ(s2num<float>("0.0"), 0.0F);
}


TEST(UtilsNumberTests, String2NumberEdgeCases)
{
    EXPECT_EQ(s2num<uint16_t>("70000", 0), 0); // out of range, returns default value
    EXPECT_EQ(s2num<int>("", -999), -999);     // empty string, returns default value
    EXPECT_EQ(s2num<int>("abc", 42), 42); // non-numeric string, returns default value
    EXPECT_EQ(s2num<int>("123abc", 0), 123);  // mixed content, does it's best
    EXPECT_EQ(s2num<int>("123.456", 0), 123); // decimal number, returns default value
    EXPECT_EQ(s2num<int>("2147483648", -1),
              -1); // out of range for int, returns default value
}


TEST(UtilsNumberTests, Digits)
{
    EXPECT_EQ(digits(0), 1);
    EXPECT_EQ(digits(1), 1);
    EXPECT_EQ(digits(9), 1);
    EXPECT_EQ(digits(10), 2);
    EXPECT_EQ(digits(99), 2);
    EXPECT_EQ(digits(100), 3);
    EXPECT_EQ(digits(999), 3);
    EXPECT_EQ(digits(1000), 4);
    EXPECT_EQ(digits(-12'345), 6); // includes the '-' sign
    EXPECT_EQ(digits(1'234'567'890), 10);
    EXPECT_EQ(digits(4'294'967'295), 10);
    EXPECT_EQ(digits(std::numeric_limits<int16_t>::max()),
              std::numeric_limits<int16_t>::digits10 + 1);
    EXPECT_EQ(digits(std::numeric_limits<int32_t>::max()),
              std::numeric_limits<int32_t>::digits10 + 1);
    EXPECT_EQ(digits(std::numeric_limits<int64_t>::max()),
              std::numeric_limits<int64_t>::digits10 + 1);
}

} // namespace