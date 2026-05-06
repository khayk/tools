#include <gtest/gtest.h>
#include <core/utils/Sys.h>

namespace {

TEST(UtilsSysTests, CurrentProcessPath)
{
    const auto path = sys::currentProcessPath();
    EXPECT_EQ(path.filename().stem().string(), "core-test");
}

} // namespace