#include <gtest/gtest.h>
#include "utils/Str.h"

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
    const auto ws = s2ws(s);

    EXPECT_EQ(ws[0], 0x0532);
    EXPECT_EQ(ws[1], 0x0561);
    EXPECT_EQ(ws[2], 0x0580);
    EXPECT_EQ(ws[3], 0x0587);
    EXPECT_EQ(ws[4], '.');
}

} // namespace
