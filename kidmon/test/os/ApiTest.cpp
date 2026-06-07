#include <gtest/gtest.h>

#include <kidmon/os/Api.h>

using namespace km;

namespace {

// Smoke tests for the activity-detection signals. These are environment
// dependent (idle time / display power state vary on CI), so they only assert
// the calls are well-formed and return sane values, not specific results.

TEST(ApiTest, IdleTimeIsNonNegative)
{
    const auto api = ApiFactory::create();
    ASSERT_TRUE(api != nullptr);

    EXPECT_GE(api->idleTime().count(), 0);
}

TEST(ApiTest, DisplayOnDoesNotThrow)
{
    const auto api = ApiFactory::create();
    ASSERT_TRUE(api != nullptr);

    EXPECT_NO_THROW(std::ignore = api->displayOn());
}

} // namespace
