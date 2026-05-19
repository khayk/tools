#include <gtest/gtest.h>
#include <core/utils/Dirs.h>

namespace fs = std::filesystem;
using namespace core::dirs;

namespace {

// Holds the absolute expected paths for each dir function on this platform.
// Computed once at startup so test cases stay #ifdef-free.
struct PlatformRelations
{
    std::optional<fs::path> expectedData;
    std::optional<fs::path> expectedCache;
    std::optional<fs::path> expectedConfig;
    // On Windows, data() and cache() both map to LocalAppData.
    bool dataSameAsCache = false;
};

PlatformRelations makePlatformRelations()
{
#ifdef _WIN32
    return {.dataSameAsCache = true};
#else
    const auto h = home();
    #ifdef __APPLE__
    return {
        .expectedData   = h / "Library/Application Support",
        .expectedCache  = h / "Library/Caches",
        .expectedConfig = h / "Library/Preferences",
    };
    #else
    return {
        .expectedData   = h / ".local/share",
        .expectedCache  = h / ".cache",
        .expectedConfig = h / ".config",
    };
    #endif
#endif
}

const PlatformRelations PLATFORM = makePlatformRelations();

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

TEST(UtilsDirsTests, DataExpectedPath)
{
    if (PLATFORM.expectedData)
    {
        EXPECT_EQ(data(), *PLATFORM.expectedData);
    }
}

TEST(UtilsDirsTests, CacheExpectedPath)
{
    if (PLATFORM.expectedCache)
    {
        EXPECT_EQ(cache(), *PLATFORM.expectedCache);
    }
}

TEST(UtilsDirsTests, ConfigExpectedPath)
{
    if (PLATFORM.expectedConfig)
    {
        EXPECT_EQ(config(), *PLATFORM.expectedConfig);
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
