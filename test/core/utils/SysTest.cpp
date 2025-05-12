#include <gtest/gtest.h>
#include <core/utils/Sys.h>

namespace {

TEST(UtilsSysTests, CurrentProcessPath)
{
    const auto path = sys::currentProcessPath();
    std::cout << "Current process path: " << path << '\n';
    EXPECT_FALSE(path.empty());

    // @todo: add more checks
}

}