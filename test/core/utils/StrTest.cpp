#include <gtest/gtest.h>
#include <core/utils/Str.h>
#include <array>

using namespace str;

namespace {

TEST(UtilsStrTests, S2WS)
{
    const auto es = s2ws("");
    EXPECT_EQ(es, L"");

    const auto ws = s2ws("simple string");
    EXPECT_EQ(ws, L"simple string");
}

TEST(UtilsStrTests, WS2S)
{
    const auto es = ws2s(L"");
    EXPECT_EQ(es, "");

    const auto ns = ws2s(L"wide string");
    EXPECT_EQ(ns, "wide string");
}

TEST(UtilsStrTests, U8S2WS)
{
    const auto s = u8"Բարև.";
    const auto ws = s2ws(u8tos(s));

    ASSERT_GE(ws.size(), 5);
    EXPECT_EQ(ws[0], 0x0532);
    EXPECT_EQ(ws[1], 0x0561);
    EXPECT_EQ(ws[2], 0x0580);
    EXPECT_EQ(ws[3], 0x0587);
    EXPECT_EQ(ws[4], '.');
}

TEST(UtilsStrTests, U8Conversions)
{
    const std::array strs = {u8"",
                             u8"Хорошо",
                             u8"๑(◕‿◕)๑",
                             u8"って行動しなければならない。",
                             u8"有對整理《周易指》感興趣的小夥伴，請聯繫我郵箱"};

    for (const auto& s : strs)
    {
        const auto ws = s2ws(u8tos(s));
        const auto ss = ws2s(ws);
        const auto s8 = stou8(ss);

        ASSERT_EQ(s, s8);
    }
}

TEST(UtilsStrTests, U8S2WSPerf)
{
    const auto s = u8"Լաւ է հանգիստ վաստակել մի բուռ, քան չարչարանքով ու տանջանքով՝ "
                   u8"երկու բուռ։";
    std::wstring ws;
    std::string ss;

    for (int i = 0; i < 10'000; ++i)
    {
        s2ws(u8tos(s), ws);
        ws2s(ws, ss);
        const auto s8 = stou8(ss);
        ASSERT_EQ(s, s8);
    }
}

TEST(UtilsStrTests, TrimLeft)
{
    std::string s = "  123  ";
    trimLeft(s);
    EXPECT_EQ(s, "123  ");

    s = "a";
    trimLeft(s);
    EXPECT_EQ(s, "a");

    s = "";
    trimLeft(s);
    EXPECT_EQ(s, "");

    s = "  \t \n  \r ";
    trimLeft(s);
    EXPECT_EQ(s, "");
}

TEST(UtilsStrTests, TrimRight)
{
    std::string s = "  123  ";
    trimRight(s);
    EXPECT_EQ(s, "  123");

    s = "a";
    trimRight(s);
    EXPECT_EQ(s, "a");

    s = "";
    trimRight(s);
    EXPECT_EQ(s, "");

    s = "  \t \n  \r ";
    trimRight(s);
    EXPECT_EQ(s, "");
}

TEST(UtilsStrTests, Trim)
{
    std::string s = "  123 a$ ";
    trim(s);
    EXPECT_EQ(s, "123 a$");

    s = "a";
    trim(s);
    EXPECT_EQ(s, "a");

    s = "";
    trim(s);
    EXPECT_EQ(s, "");

    s = "  \t \n  \r \0";
    trim(s);
    EXPECT_EQ(s, "");
}

TEST(UtilsStrTests, AsciiLowerInplace)
{
    std::string input {"SoME RANdom messaGE"};
    asciiLowerInplace(input);
    EXPECT_EQ(input, "some random message");
}

TEST(UtilsStrTests, AsciiLower)
{
    std::string input {"SoME RANdom messaGE"};
    input = asciiLower(input);
    EXPECT_EQ(input, "some random message");
}

TEST(UtilsStrTests, WStrLowerInplace)
{
    std::wstring input {L"SoME RANdom messaGE"};
    lowerInplace(input);
    EXPECT_EQ(input, L"some random message");

    input.assign(L"ՀԱՅերեն");
    lowerInplace(input);
    EXPECT_EQ(input, L"հայերեն");

    input.assign(L"Что то на РУССКОМ");
    lowerInplace(input);
    EXPECT_EQ(input, L"что то на русском");
}

TEST(UtilsStrTests, Utf8Lower)
{
    std::string input(u8tos(u8"Մակեդոնիա"));
    input = utf8Lower(input);
    const std::u8string s8(stou8(input));
    const std::u8string expected(u8"մակեդոնիա");
    // Don't check with EXPECT_EQ, because gtest is not built with c++20 support
    EXPECT_TRUE(s8 == expected);
}

} // namespace
