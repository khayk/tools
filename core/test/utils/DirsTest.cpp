#include <gtest/gtest.h>
#include <core/utils/Dirs.h>

namespace fs = std::filesystem;
using namespace core::dirs;

namespace {

// Captures platform-specific relationships between the dir functions.
// Tests use this struct so no #ifdef leaks into individual test cases.
struct PlatformRelations
{
    // Set to the expected subdirectory under home(), or nullopt if unrelated.
    std::optional<fs::path> dataFromHome;
    std::optional<fs::path> cacheFromHome;
    std::optional<fs::path> configFromHome;
    // On Windows, data() and cache() both map to LocalAppData.
    bool dataSameAsCache = false;
};

#ifdef _WIN32
constexpr PlatformRelations PLATFORM {
    .dataSameAsCache = true,
};
#else
const PlatformRelations PLATFORM {
    .dataFromHome   = ".data",
    .cacheFromHome  = ".cache",
    .configFromHome = ".config",
    .dataSameAsCache = false,
};
#endif

void checkValidPath(const fs::path& path)
{
    EXPECT_FALSE(path.empty());
    EXPECT_TRUE(path.is_absolute());
}

TEST(UtilsDirsTests, HomeIsValid)
{
    std::error_code ec;
    const auto path = home(ec);
    EXPECT_FALSE(ec) << ec.message();
    checkValidPath(path);
    EXPECT_EQ(path, home());
}

TEST(UtilsDirsTests, TempIsValid)
{
    std::error_code ec;
    const auto path = temp(ec);
    EXPECT_FALSE(ec) << ec.message();
    checkValidPath(path);
    EXPECT_EQ(path, temp());
}

TEST(UtilsDirsTests, DataIsValid)
{
    std::error_code ec;
    const auto path = data(ec);
    EXPECT_FALSE(ec) << ec.message();
    checkValidPath(path);
    EXPECT_EQ(path, data());
}

TEST(UtilsDirsTests, CacheIsValid)
{
    std::error_code ec;
    const auto path = cache(ec);
    EXPECT_FALSE(ec) << ec.message();
    checkValidPath(path);
    EXPECT_EQ(path, cache());
}

TEST(UtilsDirsTests, ConfigIsValid)
{
    std::error_code ec;
    const auto path = config(ec);
    EXPECT_FALSE(ec) << ec.message();
    checkValidPath(path);
    EXPECT_EQ(path, config());
}

TEST(UtilsDirsTests, DataRelativeToHome)
{
    if (PLATFORM.dataFromHome)
    {
        EXPECT_EQ(data(), home() / *PLATFORM.dataFromHome);
    }
}

TEST(UtilsDirsTests, CacheRelativeToHome)
{
    if (PLATFORM.cacheFromHome)
    {
        EXPECT_EQ(cache(), home() / *PLATFORM.cacheFromHome);
    }
}

TEST(UtilsDirsTests, ConfigRelativeToHome)
{
    if (PLATFORM.configFromHome)
    {
        EXPECT_EQ(config(), home() / *PLATFORM.configFromHome);
    }
}

TEST(UtilsDirsTests, DataCacheRelationship)
{
    if (PLATFORM.dataSameAsCache)
    {
        EXPECT_EQ(data(), cache());
    }
    else
    {
        EXPECT_NE(data(), cache());
    }
}

TEST(UtilsDirsTests, DirsAreDistinct)
{
    const auto h = home();
    const auto t = temp();
    const auto d = data();
    const auto c = config();

    EXPECT_NE(h, t);
    EXPECT_NE(d, c);
}

} // namespace
