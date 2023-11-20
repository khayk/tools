#include <gtest/gtest.h>
#include <patterns/Callback.h>

#include <tuple>

namespace {

TEST(CallbackTests, OneCallbackOneArg)
{
    // First argument shows number of calls
    using Info = std::tuple<int, char>;
    using CallbackOne = dp::Callback<char>;
    CallbackOne cb;
    Info info;

    auto fn = [&info](char val) {
        ++std::get<0>(info);
        std::get<1>(info) = val;
    };

    auto fp = cb.add(fn);
    cb('a');
    EXPECT_TRUE((Info {1, 'a'} == info));
    cb('b');
    EXPECT_TRUE((Info {2, 'b'} == info));

    cb.remove(fp);
    cb('c');
    EXPECT_TRUE((Info {2, 'b'} == info)); //< note, nothing is changed
}


TEST(CallbackTests, OneCallbackManyArgs)
{
    // First argument shows number of calls
    using Info = std::tuple<int, char, std::string, double>;
    using CallbackMany = dp::Callback<char, const std::string&, double>;
    CallbackMany cb;
    Info info;

    auto fn = [&info](char chr, const std::string& str, double dbl) {
        ++std::get<0>(info);
        std::get<1>(info) = chr;
        std::get<2>(info) = str;
        std::get<3>(info) = dbl;
    };

    auto fp = cb.add(fn);
    cb('Y', "es", 1.0);
    EXPECT_TRUE((Info {1, 'Y', "es", 1.0} == info));
}


TEST(CallbackTests, OneCallbackZeroArg)
{
    using CallbackZero = dp::Callback<>;
    CallbackZero cb;
    size_t calls {0};

    auto fn = [&calls]() {
        ++calls;
    };

    auto fp = cb.add(fn);
    cb();
    EXPECT_EQ(calls, 1);
}


TEST(CallbackTests, ManyCallbacks)
{
    // First argument shows number of calls
    using Info = std::tuple<int, std::string>;
    using CallbackString = dp::Callback<const std::string&>;
    CallbackString cb;

    Info info0;
    Info info1;

    auto fn0 = [&info0](const std::string& val) {
        ++std::get<0>(info0);
        std::get<1>(info0) = val;
    };

    auto fn1 = [&info1](const std::string& val) {
        ++std::get<0>(info1);
        std::get<1>(info1) = val;
    };

    auto fp0 = cb.add(fn0);
    cb("hi");
    EXPECT_TRUE((Info {1, "hi"} == info0));

    auto fp1 = cb.add(fn1);
    cb("there");
    EXPECT_TRUE((Info {2, "there"} == info0));
    EXPECT_TRUE((Info {1, "there"} == info1));

    cb.remove(fp0);
    cb("bye");
    EXPECT_TRUE((Info {2, "there"} == info0));
    EXPECT_TRUE((Info {2, "bye"} == info1));
}


TEST(CallbackTests, Performance)
{
    using CallbackString = dp::Callback<const std::string&>;
    CallbackString cb;
    size_t calls = 0;

    auto fn = [&calls](const std::string& str) {
        std::ignore = str;
        ++calls;
    };

    auto fp = cb.add(fn);
    const std::string big(1024 * 1024, '!');
    const size_t n = big.size();

    for (size_t i = 0; i < n; ++i)
    {
        cb(big);
    }

    EXPECT_EQ(calls, n);
}

} // namespace
